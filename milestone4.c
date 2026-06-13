#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "raylib.h"
#include "dijkstra.h"

#define SCREEN_WIDTH   800
#define SCREEN_HEIGHT  600
#define NODE_RADIUS    20
#define FONT_SIZE      15
#define ENTITY_RADIUS  10
#define EDGE_UNIT_TIME 0.3f
#define MAX_TRAVELERS  10

/* ── Vector2 helpers ─────────────────────────────────────────── */
Vector2 MyVector2Sub  (Vector2 a, Vector2 b)     { return (Vector2){a.x-b.x, a.y-b.y}; }
Vector2 MyVector2Add  (Vector2 a, Vector2 b)     { return (Vector2){a.x+b.x, a.y+b.y}; }
Vector2 MyVector2Scale(Vector2 v, float s)       { return (Vector2){v.x*s,   v.y*s};   }
float   MyVector2Len  (Vector2 v)                { return sqrtf(v.x*v.x + v.y*v.y);    }
Vector2 MyVector2Norm (Vector2 v) {
    float l = MyVector2Len(v);
    return l > 0.0f ? (Vector2){v.x/l, v.y/l} : (Vector2){0,0};
}
Vector2 MyVector2Lrp(Vector2 a, Vector2 b, float t) {
    return (Vector2){a.x + t*(b.x-a.x), a.y + t*(b.y-a.y)};
}

/* ── Data structures ─────────────────────────────────────────── */
typedef struct { Vector2 position; } NodeGUI;

typedef struct Traveler {
    pid_t   pid;
    int     startNode;
    int     endNode;
    int     path[MAX_NODES];
    int     pathLength;
    Color   color;

    int     currentPathIndex;
    Vector2 entityPosition;
    Vector2 startNodePos;   /* start of the edge currently being animated */
    Vector2 endNodePos;     /* end   of the edge currently being animated */
    float   travelTime;     /* total time for current edge (w * EDGE_UNIT_TIME) */
    float   elapsedTravelTime;
    int     currentEdgeWeight; /* cached – looked up once per edge, not per frame */

    bool    signaled; /* true once we have sent SIGTERM to this child */
} Traveler;

/* ── Globals ─────────────────────────────────────────────────── */
Traveler travelers[MAX_TRAVELERS];
int      numTravelers  = 0;
NodeGUI  nodePositions[MAX_NODES];
bool     animationRunning = false;

/*
 * SIGNAL DESIGN – child side
 * --------------------------
 * Goal: child stays alive until the parent sends SIGTERM (traveler arrived),
 * then prints "finished" and exits cleanly.
 *
 * Mechanism: sigaction + sigsuspend()
 *
 *   1. Install a no-op SIGTERM handler (so SIGTERM wakes sigsuspend()
 *      via EINTR instead of killing the process via the default action).
 *      SA_RESTART must be absent so sigsuspend() is not restarted.
 *
 *   2. Call sigsuspend() with a mask that blocks everything except SIGTERM.
 *      sigsuspend() atomically: sets that mask, then sleeps.
 *      When SIGTERM arrives: handler runs, sigsuspend() returns EINTR.
 *
 *   3. Fall through to printf("finished") + exit(0).
 *
 * No separate sigprocmask() needed: sigsuspend() sets the mask atomically
 * itself. The handler is installed inside run_child() (post-fork), so the
 * parent's signal disposition is never modified.
 */
static void child_sigterm_handler(int sig) { (void)sig; /* wake sigsuspend */ }

/* ── run_child: everything the child process does ────────────── */
static void run_child(void) {
    /* Install SIGTERM handler so sigsuspend() returns EINTR on delivery
     * instead of the default action (kill the process immediately).
     * SA_RESTART intentionally absent: we want sigsuspend() to be
     * interrupted, not restarted.                                        */
    struct sigaction sa;
    sa.sa_handler = child_sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags   = 0;
    sigaction(SIGTERM, &sa, NULL);

    printf("[PID=%d] started\n", getpid());
    fflush(stdout);

    /* Wait for SIGTERM and ONLY SIGTERM.
     * sigsuspend() atomically swaps in a mask that blocks everything
     * except SIGTERM, then suspends. When SIGTERM arrives the handler
     * runs, sigsuspend() returns EINTR, and we fall through.
     * No separate sigprocmask() needed – sigsuspend() is atomic.        */
    sigset_t wait_for_sigterm;
    sigfillset(&wait_for_sigterm);
    sigdelset(&wait_for_sigterm, SIGTERM);
    sigsuspend(&wait_for_sigterm);

    printf("[PID=%d] finished\n", getpid());
    fflush(stdout);
    exit(0);
}

/* ── Node layout ─────────────────────────────────────────────── */
void calculateNodePositions(int numNodes) {
    float   step   = 2.0f * PI / numNodes;
    float   radius = (SCREEN_HEIGHT < SCREEN_WIDTH ? SCREEN_HEIGHT : SCREEN_WIDTH) / 2.0f
                     - NODE_RADIUS * 2.0f;
    Vector2 center = {SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f};
    for (int i = 0; i < numNodes; i++) {
        nodePositions[i].position.x = center.x + radius * cosf(i * step);
        nodePositions[i].position.y = center.y + radius * sinf(i * step);
    }
}

/* ── Drawing ─────────────────────────────────────────────────── */
void DrawGraph(Graph* graph) {
    for (int i = 0; i < graph->numNodes; i++) {
        Vector2 src = nodePositions[i].position;
        for (Edge* e = graph->adjList[i].head; e; e = e->next) {
            Vector2 dst      = nodePositions[e->destination].position;
            Vector2 norm     = MyVector2Norm(MyVector2Sub(dst, src));
            Vector2 arrowTip = MyVector2Sub(dst, MyVector2Scale(norm, NODE_RADIUS + 5));
            float   ang      = atan2f(norm.y, norm.x);
            DrawLineEx(src, dst, 2, GRAY);
            DrawTriangle(
                arrowTip,
                MyVector2Add(arrowTip, (Vector2){-10*cosf(ang-PI/6), -10*sinf(ang-PI/6)}),
                MyVector2Add(arrowTip, (Vector2){-10*cosf(ang+PI/6), -10*sinf(ang+PI/6)}),
                GRAY);
            char wt[10]; sprintf(wt, "%d", e->weight);
            Vector2 mid = MyVector2Scale(MyVector2Add(src, dst), 0.5f);
            DrawText(wt, (int)mid.x + 5, (int)mid.y - 15, FONT_SIZE, BLACK);
        }
    }
    for (int i = 0; i < graph->numNodes; i++) {
        DrawCircleV(nodePositions[i].position, NODE_RADIUS, LIGHTGRAY);
        DrawCircleLines(nodePositions[i].position.x, nodePositions[i].position.y,
                        NODE_RADIUS, DARKGRAY);
        char txt[5]; sprintf(txt, "%d", i);
        DrawText(txt,
                 (int)(nodePositions[i].position.x - MeasureText(txt, FONT_SIZE)/2),
                 (int)(nodePositions[i].position.y - FONT_SIZE/2),
                 FONT_SIZE, BLACK);
    }
    for (int i = 0; i < numTravelers; i++) {
        if (travelers[i].pathLength == 0) continue;
        /* Draw while animating, or after the traveler has arrived (signaled) */
        bool atEnd = travelers[i].signaled ||
                     travelers[i].currentPathIndex == travelers[i].pathLength - 1;
        if (animationRunning || atEnd) {
            for (int j = 0; j < travelers[i].pathLength - 1; j++)
                DrawLineEx(nodePositions[travelers[i].path[j]].position,
                           nodePositions[travelers[i].path[j+1]].position,
                           4, travelers[i].color);
            DrawCircleV(travelers[i].entityPosition, ENTITY_RADIUS, travelers[i].color);
        }
    }
    DrawRectangle(10, 10, 80, 30, animationRunning ? RED : GREEN);
    DrawText(animationRunning ? "STOP" : "PLAY", 20, 18, 20, WHITE);
}

/* ── Helper: initialise a traveler's animation state ─────────── */
static void resetTraveler(int i) {
    travelers[i].currentPathIndex  = 0;
    travelers[i].travelTime        = 0.0f;
    travelers[i].elapsedTravelTime = 0.0f;
    travelers[i].currentEdgeWeight = 0;
    travelers[i].entityPosition    = nodePositions[travelers[i].startNode].position;
    travelers[i].startNodePos      = nodePositions[travelers[i].startNode].position;
    /* endNodePos is the true final destination of the whole trip.
     * It will be overwritten per-edge during animation, but the
     * meaningful initial value is the destination node.            */
    travelers[i].endNodePos        = nodePositions[travelers[i].endNode].position;
}

/* ── main ────────────────────────────────────────────────────── */
int main(int argc, char* argv[]) {
    if (argc < 2) { fprintf(stderr, "Usage: %s <input_file>\n", argv[0]); return 1; }

    FILE* file = fopen(argv[1], "r");
    if (!file) { perror("fopen"); return 1; }

    int numNodes, numEdges;
    fscanf(file, "%d %d", &numNodes, &numEdges);

    Graph* graph = createGraph(numNodes);
    for (int i = 0; i < numEdges; i++) {
        int u, v, w;
        fscanf(file, "%d %d %d", &u, &v, &w);
        addEdge(graph, u, v, w);
    }

    calculateNodePositions(numNodes);

    fscanf(file, "%d", &numTravelers);
    Color colors[] = {BLUE, GREEN, YELLOW, MAGENTA, ORANGE,
                      LIME, SKYBLUE, VIOLET, BEIGE, BROWN};

    for (int i = 0; i < numTravelers; i++) {
        fscanf(file, "%d %d", &travelers[i].startNode, &travelers[i].endNode);

        travelers[i].color    = colors[i % 10];
        travelers[i].signaled = false;

        int dummyWeight;
        dijkstra(graph,
                 travelers[i].startNode, travelers[i].endNode,
                 travelers[i].path, &travelers[i].pathLength, &dummyWeight);

        resetTraveler(i);   /* zero all animation fields with correct semantics */

        /* Fork the child. On failure: kill any children already running,
         * then abort — storing pid==-1 and continuing would cause
         * kill(-1, SIGTERM) at cleanup, which signals every process in
         * the process group.                                              */
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            for (int j = 0; j < i; j++) {
                kill(travelers[j].pid, SIGTERM);
                waitpid(travelers[j].pid, NULL, 0);
            }
            freeGraph(graph);
            fclose(file);
            return 1;
        }
        if (pid == 0) run_child();   /* child: never returns */
        travelers[i].pid = pid;      /* parent: record child PID */
    }
    fclose(file);

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Graph Visualization - Milestone 4");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {

        /* PLAY / STOP button */
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            CheckCollisionPointRec(GetMousePosition(), (Rectangle){10,10,80,30})) {
            animationRunning = !animationRunning;
            if (animationRunning)
                for (int i = 0; i < numTravelers; i++) resetTraveler(i);
        }

        /* Animation update */
        if (animationRunning) {
            for (int i = 0; i < numTravelers; i++) {

                /* Already at destination – nothing to do */
                if (travelers[i].currentPathIndex >= travelers[i].pathLength - 1)
                    continue;

                /* Begin a new edge: cache weight, set up start/end positions.
                 * travelTime == 0.0f is the sentinel meaning "new edge".      */
                if (travelers[i].travelTime == 0.0f) {
                    int u = travelers[i].path[travelers[i].currentPathIndex];
                    int v = travelers[i].path[travelers[i].currentPathIndex + 1];

                    int w = 0;
                    for (Edge* e = graph->adjList[u].head; e; e = e->next)
                        if (e->destination == v) { w = e->weight; break; }
                    travelers[i].currentEdgeWeight = w;  /* cached for O(1) use */

                    /* Zero-weight edge: teleport instantly, no division risk */
                    if (w == 0) {
                        travelers[i].entityPosition = nodePositions[v].position;
                        travelers[i].currentPathIndex++;
                        if (travelers[i].currentPathIndex == travelers[i].pathLength - 1
                            && !travelers[i].signaled) {
                            kill(travelers[i].pid, SIGTERM);
                            travelers[i].signaled = true;
                        }
                        continue;
                    }

                    travelers[i].travelTime        = w * EDGE_UNIT_TIME;
                    travelers[i].elapsedTravelTime = 0.0f;
                    travelers[i].startNodePos      = nodePositions[u].position;
                    travelers[i].endNodePos        = nodePositions[v].position;
                }

                travelers[i].elapsedTravelTime += GetFrameTime();

                if (travelers[i].elapsedTravelTime < travelers[i].travelTime) {
                    /* Interpolate along the edge – travelTime > 0 guaranteed */
                    float t = travelers[i].elapsedTravelTime / travelers[i].travelTime;
                    travelers[i].entityPosition =
                        MyVector2Lrp(travelers[i].startNodePos,
                                     travelers[i].endNodePos, t);
                } else {
                    /* Arrived at next node */
                    travelers[i].entityPosition    = travelers[i].endNodePos;
                    travelers[i].travelTime        = 0.0f;  /* sentinel for next edge */
                    travelers[i].elapsedTravelTime = 0.0f;
                    travelers[i].currentPathIndex++;

                    /* Reached final destination: terminate the child exactly once */
                    if (travelers[i].currentPathIndex == travelers[i].pathLength - 1
                        && !travelers[i].signaled) {
                        kill(travelers[i].pid, SIGTERM);
                        travelers[i].signaled = true;
                    }
                }
            }
        }

        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawGraph(graph);
        EndDrawing();
    }

    /*
     * Cleanup: one loop, no redundancy.
     * For each child: send SIGTERM only if not already sent,
     * then waitpid() unconditionally (every child needs to be reaped).
     * waitpid() on a child that already exited after SIGTERM returns
     * immediately – it does not block.
     */
    for (int i = 0; i < numTravelers; i++) {
        if (travelers[i].pid > 0) {
            if (!travelers[i].signaled)
                kill(travelers[i].pid, SIGTERM);
            waitpid(travelers[i].pid, NULL, 0);
        }
    }

    freeGraph(graph);
    CloseWindow();
    return 0;
}