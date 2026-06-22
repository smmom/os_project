#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h> /* For strcasecmp */
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>
#include <errno.h>
#include "raylib.h"
#include "dijkstra.h"

/* ─── screen / drawing constants ────────────────────────────────── */
#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  600
#define NODE_RADIUS    20
#define FONT_SIZE      15
#define ENTITY_RADIUS  10
#define MAX_TRAVELERS  10

/* ─── semaphore name helpers ─────────────────────────────────────── */
#define NODE_SEM_FMT  "/ms7_n_%d"
#define ENTER_SEM_FMT "/ms7_e_%d"
#define CONT_SEM_FMT  "/ms7_c_%d"

/* ─── scheduling algorithm ───────────────────────────────────────── */
typedef enum { FCFS, SJF } SchedulingAlgo;
static SchedulingAlgo current_algo = FCFS;
static char           algo_name[8] = "FCFS";

/* ─── IPC message types (child → parent, via pipe) ───────────────── */
typedef enum {
    MSG_REQUEST = 1,   /* child requests entry to node    */
    MSG_ARRIVED,       /* child formally entered node     */
    MSG_LEFT,          /* child left node (after sleep)   */
    MSG_FINISHED       /* child reached final destination */
} MsgType;

typedef struct {
    MsgType   type;
    pid_t     pid;
    int       traveler_idx;       
    int       node;               
    int       next_node;          
    long long arrival_time_ms;    
    int       next_seg_weight;    
} IPCMessage;

/* ─── waiting queue entry ────────────────────────────────────────── */
typedef struct WaitingEntry {
    int       traveler_idx;
    long long arrival_time_ms;
    int       next_seg_weight;
    struct WaitingEntry *next;
} WaitingEntry;

/* ─── per-node queue ─────────────────────────────────────────────── */
typedef struct {
    WaitingEntry *head;           
} NodeQueue;

static NodeQueue *node_queues = NULL;   
static int       *node_occupants = NULL; /* Tracks strictly who is inside */
static int        total_nodes = 0;

/* Semaphores */
static sem_t **node_sems     = NULL;   
static sem_t **enter_sems    = NULL;   
static sem_t **continue_sems = NULL;   
static int     num_travelers_global = 0;

/* Pipe */
static int pipefd[2];

/* ─── Vector2 helpers ────────────────────────────────────────────── */
static inline Vector2 V2Sub  (Vector2 a,Vector2 b){return(Vector2){a.x-b.x,a.y-b.y};}
static inline Vector2 V2Add  (Vector2 a,Vector2 b){return(Vector2){a.x+b.x,a.y+b.y};}
static inline Vector2 V2Scale(Vector2 v,float s)  {return(Vector2){v.x*s,  v.y*s};  }
static inline float   V2Len  (Vector2 v)          {return sqrtf(v.x*v.x+v.y*v.y);   }
static inline Vector2 V2Norm (Vector2 v){
    float l=V2Len(v); return l>0.f?(Vector2){v.x/l,v.y/l}:(Vector2){0,0};
}
static inline Vector2 V2Lerp(Vector2 a,Vector2 b,float t){
    return(Vector2){a.x+t*(b.x-a.x),a.y+t*(b.y-a.y)};
}

/* ─── GUI node position ──────────────────────────────────────────── */
typedef struct { Vector2 position; } NodeGUI;
static NodeGUI nodePositions[MAX_NODES];

/* ─── Traveler (parent-side state) ──────────────────────────────── */
typedef struct {
    pid_t   pid;
    int     startNode, endNode;
    int     path[MAX_NODES];
    int     pathLength;
    Color   color;
    bool    active;
    bool    waiting;           
    Vector2 entityPosition;
    int     fromNode, toNode;
    float   moveProgress;      
    float   moveDuration;      
} Traveler;

static Traveler travelers[MAX_TRAVELERS];
static int      numTravelers = 0;

static enum { STATE_IDLE, STATE_RUNNING, STATE_PAUSED } animState = STATE_IDLE;

/* ═══════════════════════════════════════════════════════════════════
 * Safe Wrappers (Addresses EINTR & Write validations)
 * ═══════════════════════════════════════════════════════════════════ */
static void safe_sem_wait(sem_t *s) {
    while (sem_wait(s) < 0) {
        if (errno != EINTR) { perror("sem_wait"); exit(EXIT_FAILURE); }
    }
}

static void safe_write(int fd, const void *buf, size_t count) {
    if (write(fd, buf, count) < 0) { perror("write pipe"); exit(EXIT_FAILURE); }
}

/* ═══════════════════════════════════════════════════════════════════
 * Queue helpers (Fixes Bug 6)
 * ═══════════════════════════════════════════════════════════════════ */
static void queue_add(int node, int tidx, long long arr_ms, int seg_w) {
    WaitingEntry *e = malloc(sizeof(WaitingEntry));
    if (!e) { perror("malloc"); return; }
    e->traveler_idx    = tidx;
    e->arrival_time_ms = arr_ms;
    e->next_seg_weight = seg_w;
    e->next            = NULL;

    WaitingEntry **pp = &node_queues[node].head;
    while (*pp) pp = &(*pp)->next;
    *pp = e;
}

static WaitingEntry *queue_pop(int node) {
    WaitingEntry *head = node_queues[node].head;
    if (!head) return NULL;

    WaitingEntry *best_prev = NULL, *best = head;
    WaitingEntry *prev      = head, *cur  = head->next;

    while (cur) {
        bool pick = false;
        if (current_algo == FCFS) {
            if (cur->arrival_time_ms < best->arrival_time_ms) pick = true;
        } else { /* SJF */
            if (cur->next_seg_weight < best->next_seg_weight) pick = true;
            else if (cur->next_seg_weight == best->next_seg_weight &&
                     cur->arrival_time_ms < best->arrival_time_ms) pick = true;
        }
        if (pick) { best_prev = prev; best = cur; }
        prev = cur; cur = cur->next;
    }

    if (best_prev) best_prev->next = best->next;
    else           node_queues[node].head = best->next;
    best->next = NULL;
    return best;
}

static void queues_free(void) {
    if (!node_queues) return;
    for (int i = 0; i < total_nodes; i++) {
        WaitingEntry *e = node_queues[i].head;
        while (e) { WaitingEntry *nx = e->next; free(e); e = nx; }
    }
    free(node_queues);
}

/* ═══════════════════════════════════════════════════════════════════
 * Semaphore helpers (Fixes Bug 4 & 8)
 * ═══════════════════════════════════════════════════════════════════ */
static void sems_create(int n_nodes, int n_travelers) {
    node_sems = calloc(n_nodes, sizeof(sem_t *));
    for (int i = 0; i < n_nodes; i++) {
        char name[64]; sprintf(name, NODE_SEM_FMT, i);
        sem_unlink(name);
        node_sems[i] = sem_open(name, O_CREAT|O_EXCL, 0644, 1);
        if (node_sems[i] == SEM_FAILED) { perror("sem_open node"); exit(1); }
    }

    enter_sems = calloc(n_travelers, sizeof(sem_t *));
    continue_sems = calloc(n_travelers, sizeof(sem_t *));
    for (int i = 0; i < n_travelers; i++) {
        char ename[64], cname[64];
        sprintf(ename, ENTER_SEM_FMT, i);
        sprintf(cname, CONT_SEM_FMT, i);
        sem_unlink(ename); sem_unlink(cname);
        enter_sems[i] = sem_open(ename, O_CREAT|O_EXCL, 0644, 0);
        continue_sems[i] = sem_open(cname, O_CREAT|O_EXCL, 0644, 0);
        if (enter_sems[i] == SEM_FAILED || continue_sems[i] == SEM_FAILED) { 
            perror("sem_open traveler"); exit(1); 
        }
    }
}

static void sems_destroy(int n_nodes, int n_travelers) {
    for (int i = 0; i < n_nodes; i++) {
        char name[64]; sprintf(name, NODE_SEM_FMT, i);
        if (node_sems && node_sems[i]) { sem_close(node_sems[i]); sem_unlink(name); }
    }
    free(node_sems);

    for (int i = 0; i < n_travelers; i++) {
        char ename[64], cname[64];
        sprintf(ename, ENTER_SEM_FMT, i);
        sprintf(cname, CONT_SEM_FMT, i);
        if (enter_sems && enter_sems[i]) { sem_close(enter_sems[i]); sem_unlink(ename); }
        if (continue_sems && continue_sems[i]) { sem_close(continue_sems[i]); sem_unlink(cname); }
    }
    free(enter_sems); free(continue_sems);
}

static long long now_ms(void) {
    struct timeval tv; gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + (long long)tv.tv_usec / 1000;
}

/* ═══════════════════════════════════════════════════════════════════
 * Child process – walk path with parent-controlled scheduling
 * ═══════════════════════════════════════════════════════════════════ */
static void run_child(Graph *graph, int tidx) {
    close(pipefd[0]);   

    char esem_name[64], csem_name[64];
    sprintf(esem_name, ENTER_SEM_FMT, tidx);
    sprintf(csem_name, CONT_SEM_FMT, tidx);
    sem_t *esem = sem_open(esem_name, 0);
    sem_t *csem = sem_open(csem_name, 0);
    if (esem == SEM_FAILED || csem == SEM_FAILED) { perror("child sem"); exit(1); }

    int p[MAX_NODES], len, total_w;
    if (!dijkstra(graph, travelers[tidx].startNode, travelers[tidx].endNode, p, &len, &total_w)) {
        IPCMessage f = {MSG_FINISHED, getpid(), tidx, -1, -1, 0, 0};
        safe_write(pipefd[1], &f, sizeof(f));
        sem_close(esem); sem_close(csem); close(pipefd[1]);
        exit(0);
    }

    for (int j = 0; j < len; j++) {
        int cur_node  = p[j];
        int nxt_node  = (j < len-1) ? p[j+1] : -1;
        int seg_w = 0;
        
        if (nxt_node != -1) {
            for (Edge *e = graph->adjList[cur_node].head; e; e = e->next)
                if (e->destination == nxt_node) { seg_w = e->weight; break; }
        }

        char nsem_name[64];
        sprintf(nsem_name, NODE_SEM_FMT, cur_node);
        sem_t *nsem = sem_open(nsem_name, 0);
        
        /* 1. Request entry from the strict parent monitor */
        IPCMessage req = {MSG_REQUEST, getpid(), tidx, cur_node, nxt_node, now_ms(), seg_w};
        safe_write(pipefd[1], &req, sizeof(req));

        /* 2. Wait for parent to give us explicit permission */
        safe_sem_wait(esem);

        /* 3. Formally acquire the OS token. Because we hold `esem`, this is guaranteed instantly */
        safe_sem_wait(nsem);

        /* 4. We officially own the node semaphore now */
        IPCMessage am = {MSG_ARRIVED, getpid(), tidx, cur_node, nxt_node, 0, seg_w};
        safe_write(pipefd[1], &am, sizeof(am));

        /* 5. Occupy node */
        sleep(1);

        /* 6. EXPLICITLY RELEASE ownership (Fixes Bug 1 & 2) */
        sem_post(nsem);

        /* 7. Tell parent we are done so it can trigger the queue */
        IPCMessage lm = {MSG_LEFT, getpid(), tidx, cur_node, nxt_node, 0, 0};
        safe_write(pipefd[1], &lm, sizeof(lm));

        /* 8. Wait for parent ACK before traveling edge */
        safe_sem_wait(csem);

        sem_close(nsem);
        if (nxt_node != -1) usleep(seg_w * 300000);
    }

    IPCMessage fm = {MSG_FINISHED, getpid(), tidx, -1, -1, 0, 0};
    safe_write(pipefd[1], &fm, sizeof(fm));

    sem_close(esem); sem_close(csem); close(pipefd[1]);
    exit(0);
}

/* ═══════════════════════════════════════════════════════════════════
 * Start / Pause / Resume / Kill
 * ═══════════════════════════════════════════════════════════════════ */
static void StartTravelers(Graph *graph) {
    for (int i = 0; i < numTravelers; i++) {
        pid_t pid = fork();
        if (pid < 0)       { perror("fork"); continue; }
        if (pid == 0)      { run_child(graph, i); }
        travelers[i].pid = pid;
    }
}

static void PauseTravelers(void) {
    for (int i = 0; i < numTravelers; i++)
        if (travelers[i].pid > 0) kill(travelers[i].pid, SIGSTOP);
}

static void ResumeTravelers(void) {
    for (int i = 0; i < numTravelers; i++)
        if (travelers[i].pid > 0) kill(travelers[i].pid, SIGCONT);
}

static void KillAllTravelers(void) {
    for (int i = 0; i < numTravelers; i++) {
        if (travelers[i].pid > 0) {
            kill(travelers[i].pid, SIGKILL);
            waitpid(travelers[i].pid, NULL, 0);
            travelers[i].pid = 0;
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════
 * Visuals
 * ═══════════════════════════════════════════════════════════════════ */
static void calculateNodePositions(int n) {
    float step   = 2.0f * PI / (float)n;
    float radius = (SCREEN_HEIGHT < SCREEN_WIDTH ? SCREEN_HEIGHT : SCREEN_WIDTH)
                   / 2.0f - NODE_RADIUS * 2.0f;
    Vector2 ctr  = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };
    for (int i = 0; i < n; i++) {
        nodePositions[i].position.x = ctr.x + radius * cosf((float)i * step);
        nodePositions[i].position.y = ctr.y + radius * sinf((float)i * step);
    }
}

static void DrawGraph(Graph *graph) {
    for (int i = 0; i < graph->numNodes; i++) {
        Vector2 src = nodePositions[i].position;
        for (Edge *e = graph->adjList[i].head; e; e = e->next) {
            Vector2 dst  = nodePositions[e->destination].position;
            Vector2 norm = V2Norm(V2Sub(dst, src));
            Vector2 tip  = V2Sub(dst, V2Scale(norm, NODE_RADIUS + 5));
            float   ang  = atan2f(norm.y, norm.x);
            DrawLineEx(src, dst, 2, GRAY);
            DrawTriangle(tip,
                V2Add(tip,(Vector2){-10*cosf(ang-PI/6),-10*sinf(ang-PI/6)}),
                V2Add(tip,(Vector2){-10*cosf(ang+PI/6),-10*sinf(ang+PI/6)}),
                GRAY);
            char wt[12]; sprintf(wt, "%d", e->weight);
            Vector2 mid = V2Scale(V2Add(src, dst), 0.5f);
            DrawText(wt, (int)mid.x + 5, (int)mid.y - 15, FONT_SIZE, BLACK);
        }
    }

    for (int i = 0; i < numTravelers; i++) {
        if (!travelers[i].active) continue;
        for (int j = 0; j < travelers[i].pathLength - 1; j++)
            DrawLineEx(nodePositions[travelers[i].path[j]].position,
                       nodePositions[travelers[i].path[j+1]].position,
                       4, travelers[i].color);
        if (travelers[i].waiting) {
            int bw = MeasureText("WAIT", 10);
            DrawRectangle((int)(travelers[i].entityPosition.x - bw/2 - 2),
                          (int)(travelers[i].entityPosition.y - ENTITY_RADIUS - 18),
                          bw + 4, 14, RED);
            DrawText("WAIT", (int)(travelers[i].entityPosition.x - bw/2),
                     (int)(travelers[i].entityPosition.y - ENTITY_RADIUS - 16), 10, WHITE);
        }
        DrawCircleV(travelers[i].entityPosition, ENTITY_RADIUS, travelers[i].color);
        DrawCircleLines(travelers[i].entityPosition.x, travelers[i].entityPosition.y, ENTITY_RADIUS, BLACK);
    }

    for (int i = 0; i < graph->numNodes; i++) {
        DrawCircleV(nodePositions[i].position, NODE_RADIUS, LIGHTGRAY);
        DrawCircleLines(nodePositions[i].position.x, nodePositions[i].position.y, NODE_RADIUS, DARKGRAY);
        char txt[5]; sprintf(txt, "%d", i);
        DrawText(txt, (int)(nodePositions[i].position.x - MeasureText(txt,FONT_SIZE)/2),
                 (int)(nodePositions[i].position.y - FONT_SIZE/2), FONT_SIZE, BLACK);
    }

    const char *btnT = "PLAY"; Color btnC = GREEN;
    if (animState == STATE_RUNNING) { btnT = "PAUSE"; btnC = RED;   }
    if (animState == STATE_PAUSED)  { btnT = "PLAY";  btnC = GREEN; }
    DrawRectangle(10, 10, 110, 30, btnC);
    DrawText(btnT, 10 + (110 - MeasureText(btnT, 20))/2, 15, 20, WHITE);

    char label[32]; sprintf(label, "Algorithm: %s", algo_name);
    DrawRectangle(10, 50, MeasureText(label, 18) + 10, 24, DARKBLUE);
    DrawText(label, 15, 53, 18, WHITE);
}

/* ═══════════════════════════════════════════════════════════════════
 * main
 * ═══════════════════════════════════════════════════════════════════ */
int main(int argc, char *argv[]) {
    const char *input_file = NULL;
    if (argc == 4) {
        if (strcmp(argv[1], "-schd") != 0) {
            fprintf(stderr, "Usage: %s [-schd fcfs|sjf] <input_file>\n", argv[0]); return 1;
        }
        if (strcasecmp(argv[2], "sjf") == 0) {
            current_algo = SJF; strncpy(algo_name, "SJF", sizeof(algo_name)-1);
        } else if (strcasecmp(argv[2], "fcfs") == 0) {
            current_algo = FCFS; strncpy(algo_name, "FCFS", sizeof(algo_name)-1);
        } else {
            fprintf(stderr, "Unknown algo '%s'\n", argv[2]); return 1;
        }
        input_file = argv[3];
    } else if (argc == 2) {
        input_file = argv[1];
    } else {
        fprintf(stderr, "Usage: %s [-schd fcfs|sjf] <input_file>\n", argv[0]); return 1;
    }

    printf("[sim] Scheduling: %s\n", algo_name);

    FILE *file = fopen(input_file, "r");
    if (!file) { perror("fopen"); return 1; }

    int numNodes, numEdges;
    if (fscanf(file, "%d %d", &numNodes, &numEdges) != 2 || numNodes <= 0 || numNodes > MAX_NODES) {
        fprintf(stderr, "Invalid graph header\n"); fclose(file); return 1;
    }

    Graph *graph = createGraph(numNodes);
    for (int i = 0; i < numEdges; i++) {
        int u, v, w;
        if (fscanf(file, "%d %d %d", &u, &v, &w) != 3) {
            fprintf(stderr, "Bad edge %d\n", i); freeGraph(graph); fclose(file); return 1;
        }
        addEdge(graph, u, v, w);
    }
    calculateNodePositions(numNodes);

    if (fscanf(file, "%d", &numTravelers) != 1 || numTravelers <= 0 || numTravelers > MAX_TRAVELERS) {
        fprintf(stderr, "Invalid traveler count\n"); freeGraph(graph); fclose(file); return 1;
    }
    num_travelers_global = numTravelers;

    Color colors[] = { BLUE, GREEN, YELLOW, MAGENTA, ORANGE, LIME, SKYBLUE, VIOLET, BEIGE, BROWN };
    for (int i = 0; i < numTravelers; i++) {
        if (fscanf(file, "%d %d", &travelers[i].startNode, &travelers[i].endNode) != 2) {
            fprintf(stderr, "Bad traveler %d\n", i); freeGraph(graph); fclose(file); return 1;
        }
        travelers[i].color = colors[i % 10]; travelers[i].active = true; travelers[i].waiting = false;
        travelers[i].pid = 0; travelers[i].pathLength = 0;
        travelers[i].fromNode = travelers[i].startNode; travelers[i].toNode = travelers[i].startNode;
        travelers[i].moveProgress = 1.0f; travelers[i].moveDuration = 0.3f;
        travelers[i].entityPosition = nodePositions[travelers[i].startNode].position;

        int dw;
        dijkstra(graph, travelers[i].startNode, travelers[i].endNode, travelers[i].path, &travelers[i].pathLength, &dw);
    }
    fclose(file);

    if (pipe(pipefd) == -1) { perror("pipe"); freeGraph(graph); return 1; }
    if (fcntl(pipefd[0], F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl"); close(pipefd[0]); close(pipefd[1]); freeGraph(graph); return 1;
    }

    total_nodes = numNodes;
    node_queues = calloc(total_nodes, sizeof(NodeQueue));
    
    /* Strict Parent State Tracker */
    node_occupants = calloc(total_nodes, sizeof(int));
    for (int i = 0; i < total_nodes; i++) node_occupants[i] = -1;

    sems_create(total_nodes, numTravelers);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Graph Traffic Simulation – Milestone 7");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), (Rectangle){10,10,110,30})) {
            if      (animState==STATE_IDLE)    { animState=STATE_RUNNING; StartTravelers(graph); }
            else if (animState==STATE_RUNNING) { animState=STATE_PAUSED;  PauseTravelers();      }
            else if (animState==STATE_PAUSED)  { animState=STATE_RUNNING; ResumeTravelers();     }
        }

        if (animState == STATE_RUNNING) {
            IPCMessage m; ssize_t br;
            /* Ensures atomicity by enforcing full struct read */
            while ((br = read(pipefd[0], &m, sizeof(m))) > 0) {
                if (br != sizeof(m)) continue; 
                int ti = m.traveler_idx;
                if (ti < 0 || ti >= numTravelers) continue;

                switch (m.type) {
                case MSG_REQUEST:
                    travelers[ti].waiting = true;
                    travelers[ti].entityPosition = nodePositions[m.node].position;
                    
                    /* The Parent acts as the strict monitor / arbiter */
                    if (node_occupants[m.node] == -1) {
                        /* Node free, grant access immediately */
                        node_occupants[m.node] = ti;
                        sem_post(enter_sems[ti]);
                    } else {
                        /* Node busy, push to logic queue */
                        queue_add(m.node, ti, m.arrival_time_ms, m.next_seg_weight);
                    }
                    break;

                case MSG_ARRIVED:
                    travelers[ti].waiting = false;
                    if (m.next_node == -1) {
                        travelers[ti].entityPosition = nodePositions[m.node].position;
                        travelers[ti].moveProgress = 1.0f;
                    } else {
                        travelers[ti].fromNode = m.node; travelers[ti].toNode = m.next_node;
                        travelers[ti].moveProgress = 0.0f;
                        travelers[ti].moveDuration = (float)(m.next_seg_weight > 0 ? m.next_seg_weight : 1) * 0.3f;
                    }
                    break;

                case MSG_LEFT:
                    /* State cleanup */
                    node_occupants[m.node] = -1;
                    /* Acknowledge departure, allow child to traverse edge */
                    sem_post(continue_sems[ti]);

                    /* Process queued threads */
                    WaitingEntry *winner = queue_pop(m.node);
                    if (winner) {
                        node_occupants[m.node] = winner->traveler_idx;
                        sem_post(enter_sems[winner->traveler_idx]);
                        free(winner);
                    }
                    break;

                case MSG_FINISHED:
                    travelers[ti].active = false;
                    waitpid(travelers[ti].pid, NULL, 0);
                    travelers[ti].pid = 0;
                    break;
                }
            }

            float dt = GetFrameTime();
            for (int i = 0; i < numTravelers; i++) {
                if (!travelers[i].active || travelers[i].moveProgress >= 1.0f) continue;
                travelers[i].moveProgress += dt / travelers[i].moveDuration;
                if (travelers[i].moveProgress > 1.0f) travelers[i].moveProgress = 1.0f;
                travelers[i].entityPosition = V2Lerp(
                    nodePositions[travelers[i].fromNode].position,
                    nodePositions[travelers[i].toNode].position, travelers[i].moveProgress);
            }
        }
        BeginDrawing(); ClearBackground(RAYWHITE); DrawGraph(graph); EndDrawing();
    }

    KillAllTravelers();
    close(pipefd[0]);
    free(node_occupants);
    sems_destroy(total_nodes, numTravelers);
    queues_free(); freeGraph(graph); CloseWindow();
    return 0;
}
