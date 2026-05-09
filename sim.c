#include <stdio.h>
#include <math.h>
#include "raylib.h"
#include "dijkstra.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define NODE_RADIUS 20
#define FONT_SIZE 15
#define ENTITY_RADIUS 10
#define ENTITY_COLOR RED
#define NODE_HOLD_TIME 1.0f // 1 second
#define EDGE_UNIT_TIME 0.3f // 300 milliseconds per unit of weight


Vector2 positions[MAX_NODES];
// Global variables for Dijkstra's path and animation

int path[MAX_NODES];
int pathLen = 0;
int totalWeight = 0;

// Animation state
bool animationRunning = true;
int currentPathIndex = 0;
float animationTimer = 0.0f;
Vector2 entityPosition;
Vector2 startNodePos;
Vector2 endNodePos;


// Animation state (new improved)
int currentIndex = 0;
int step;
int totalSteps;
float timer;
float waitTimer = 0;
bool waiting;
bool playing = true;

/* ===== get weight ===== */
int getWeight(Graph* g, int u, int v) {
    for (Edge* e = g->adjList[u].head; e; e = e->next)
        if (e->destination == v) return e->weight;
    return 1;
}

/* ===== layout ===== */
void layout(Graph* g) {
    float angle = 2 * PI / g->numNodes;

    for (int i = 0; i < g->numNodes; i++) {
        positions[i].x = SCREEN_WIDTH/2 + 200*cosf(i*angle);
        positions[i].y = SCREEN_HEIGHT/2 + 200*sinf(i*angle);
    }
}

void drawArrow(Vector2 a, Vector2 b) {
    DrawLineV(a, b, BLACK);

    // simple arrow head (no raymath)
    float dx = b.x - a.x;
    float dy = b.y - a.y;

    float len = sqrtf(dx*dx + dy*dy);
    if (len == 0) return;

    dx /= len;
    dy /= len;

    Vector2 left = {b.x - dx*10 + dy*5, b.y - dy*10 - dx*5};
    Vector2 right = {b.x - dx*10 - dy*5, b.y - dy*10 + dx*5};

    DrawTriangle(b, left, right, BLACK);
}

void DrawGraph(Graph* graph) {

    for (int i = 0; i < graph->numNodes; i++) {

        for (Edge* e = graph->adjList[i].head; e; e = e->next) {

            drawArrow(positions[i], positions[e->destination]);

            Vector2 mid = {
                (positions[i].x + positions[e->destination].x)/2,
                (positions[i].y + positions[e->destination].y)/2
            };

            char w[10];
            sprintf(w, "%d", e->weight);
            DrawText(w, mid.x, mid.y, 15, BLACK);
        }
    }

    for (int i = 0; i < graph->numNodes; i++) {
        DrawCircleV(positions[i], NODE_RADIUS, LIGHTGRAY);

        char txt[10];
        sprintf(txt, "%d", i);
        DrawText(txt, positions[i].x - 5, positions[i].y - 5, 20, BLACK);
    }

    for (int i = 0; i < pathLen - 1; i++) {
        DrawLineV(positions[path[i]], positions[path[i+1]], BLUE);
    }

    if (pathLen > 0)
        DrawCircleV(entityPosition, ENTITY_RADIUS, RED);
}

int main(int argc, char* argv[]) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Graph Visualization - Milestone 3");
    SetTargetFPS(60);

    // char* filename = "input.txt";
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;

    }

    FILE* file = fopen(argv[1], "r");
    if (!file) {
        perror("Error opening input file");
        fclose(file);
        return -1;
    }
    int N,M;
    fscanf(file,"%d %d",&N,&M);

    Graph* g = createGraph(N);

    for(int i=0;i<M;i++){
        int u,v,w;
        fscanf(file,"%d %d %d",&u,&v,&w);
        addEdge(g,u,v,w);
    }

    int s,e;
    fscanf(file,"%d %d",&s,&e);

    dijkstra(g,s,e,path,&pathLen,&totalWeight);

    layout(g);
    if (pathLen>0)
        entityPosition = positions[path[0]];

    while (!WindowShouldClose()) {

        float dt = GetFrameTime();

        /* ===== BUTTON ===== */
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 m = GetMousePosition();
            if (CheckCollisionPointRec(m,(Rectangle){10,10,80,30}))
                playing = !playing;
        }

        /* ===== ANIMATION ===== */
        if (playing && pathLen > 1 && currentIndex < pathLen-1) {

            int u = path[currentIndex];
            int v = path[currentIndex+1];

            Vector2 start = positions[u];
            Vector2 end   = positions[v];

            if (waiting) {
                waitTimer += dt;

                if (waitTimer >= NODE_HOLD_TIME) {
                    waiting = false;
                    waitTimer = 0;
                    step = 0;
                    totalSteps = getWeight(g,u,v);
                }
            }
            else {

                if (totalSteps == 0) {
                    totalSteps = getWeight(g,u,v);
                }

                timer += dt;

                if (timer >= EDGE_UNIT_TIME) {
                    timer = 0;
                    step++;

                    float t = (float)step / totalSteps;

                    entityPosition.x = start.x + (end.x - start.x)*t;
                    entityPosition.y = start.y + (end.y - start.y)*t;

                    if (step >= totalSteps) {
                        entityPosition = end;
                        currentIndex++;
                        totalSteps = 0;

                        if (currentIndex < pathLen-1)
                            waiting = true;
                    }
                }
            }
        }

        /* ===== DRAW ===== */
        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawGraph(g);

        // button
        DrawRectangle(10,10,80,30, playing?RED:GREEN);
        DrawText(playing?"STOP":"PLAY",20,15,20,WHITE);

        // arrival
        if (currentIndex == pathLen-1 && pathLen>0)
            DrawText("Arrived!", SCREEN_WIDTH/2-50, 20, 20, BLUE);

        EndDrawing();
    }

    freeGraph(g);
    CloseWindow();

    return 0;
}
