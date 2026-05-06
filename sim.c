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



// // Function to calculate node positions to avoid overlap
// void calculateNodePositions(Graph* graph) {
//     float angle_step = 2 * PI / graph->numNodes;
//     float radius = (SCREEN_HEIGHT < SCREEN_WIDTH ? SCREEN_HEIGHT : SCREEN_WIDTH) / 2 - NODE_RADIUS * 2;
//     Vector2 center = (Vector2){SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};
//
//     for (int i = 0; i < graph->numNodes; i++) {
//         graph->adjList[i].position.x = center.x + radius * cos(i * angle_step);
//         graph->adjList[i].position.y = center.y + radius * sin(i * angle_step);
//     }
// }
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


// Main drawing function
// void DrawGraph(Graph* graph) {
//     // Draw edges first
//     for (int i = 0; i < graph->numNodes; i++) {
//         Vector2 startPos = graph->adjList[i].position;
//         Edge* currentEdge = graph->adjList[i].head;
//         while (currentEdge != NULL) {
//             Vector2 endPos = graph->adjList[currentEdge->destination].position;
//
//             // Draw arrow line
//             DrawLineEx(startPos, endPos, 2, GRAY);
//
//             // Draw arrow head (simple triangle)
//             Vector2 diff = Vector2Subtract(endPos, startPos);
//             Vector2 normDiff = Vector2Normalize(diff);
//             Vector2 arrowBase = Vector2Subtract(endPos, Vector2Scale(normDiff, NODE_RADIUS + 5));
//
//             float angle = atan2f(normDiff.y, normDiff.x);
//             Vector2 p1 = arrowBase;
//             Vector2 p2 = Vector2Add(arrowBase, (Vector2){-10 * cosf(angle - PI/6), -10 * sinf(angle - PI/6)});
//             Vector2 p3 = Vector2Add(arrowBase, (Vector2){-10 * cosf(angle + PI/6), -10 * sinf(angle + PI/6)});
//             DrawTriangle(p1, p2, p3, GRAY);
//
//             // Draw weight
//             Vector2 midPoint = Vector2Scale(Vector2Add(startPos, endPos), 0.5f);
//             char weightText[10];
//             sprintf(weightText, "%d", currentEdge->weight);
//             DrawText(weightText, midPoint.x + 5, midPoint.y - 15, FONT_SIZE, BLACK);
//
//             currentEdge = currentEdge->next;
//         }
//     }
//
//     // Draw nodes
//     for (int i = 0; i < graph->numNodes; i++) {
//         Vector2 pos = graph->adjList[i].position;
//         DrawCircleV(pos, NODE_RADIUS, LIGHTGRAY);
//         DrawCircleLines(pos.x, pos.y, NODE_RADIUS, DARKGRAY);
//
//         char nodeText[5];
//         sprintf(nodeText, "%d", i);
//         DrawText(nodeText, pos.x - MeasureText(nodeText, FONT_SIZE) / 2, pos.y - FONT_SIZE / 2, FONT_SIZE, BLACK);
//     }
//
//     // Draw shortest path if animation is running or completed
//     if (shortestPathLength > 0) {
//         for (int i = 0; i < shortestPathLength - 1; i++) {
//             Vector2 p1 = graph->adjList[shortestPath[i]].position;
//             Vector2 p2 = graph->adjList[shortestPath[i+1]].position;
//             DrawLineEx(p1, p2, 4, BLUE);
//         }
//     }
//
//     // Draw entity
//     if (animationRunning || currentPathIndex == shortestPathLength - 1) {
//         DrawCircleV(entityPosition, ENTITY_RADIUS, ENTITY_COLOR);
//     }
//
//     // Draw play/stop button
//     DrawRectangle(10, 10, 80, 30, animationRunning ? RED : GREEN);
//     DrawText(animationRunning ? "STOP" : "PLAY", 20, 18, 20, WHITE);
//
//     // Draw arrival message
//     if (currentPathIndex == shortestPathLength - 1 && shortestPathLength > 0) {
//         DrawText("Arrived at destination!", SCREEN_WIDTH / 2 - MeasureText("Arrived at destination!", 20) / 2, SCREEN_HEIGHT - 30, 20, BLUE);
//     }
// }

int main(int argc, char* argv[]) {
//     // Initialization
//     InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Graph Visualization - Milestone 3");
//     SetTargetFPS(60);
//
//     char* filename = "input.txt";
//     if (argc > 1) {
//         filename = argv[1];
//     }
//
//     FILE* file = fopen(filename, "r");
//     if (!file) {
//         perror("Error opening input file");
//         return EXIT_FAILURE;
//     }
//
//     int numNodes, numEdges;
//     if (fscanf(file, "%d %d", &numNodes, &numEdges) != 2) {
//         fprintf(stderr, "Error reading number of nodes and edges.\n");
//         fclose(file);
//         return EXIT_FAILURE;
//     }
//
//     if (numNodes > MAX_NODES) {
//         fprintf(stderr, "Number of nodes exceeds MAX_NODES (%d).\n", MAX_NODES);
//         fclose(file);
//         return EXIT_FAILURE;
//     }
//
//     Graph* graph = createGraph(numNodes);
//
//     for (int i = 0; i < numEdges; i++) {
//         int src, dest, weight;
//         if (fscanf(file, "%d %d %d", &src, &dest, &weight) != 3) {
//             fprintf(stderr, "Error reading edge data.\n");
//             freeGraph(graph);
//             fclose(file);
//             return EXIT_FAILURE;
//         }
//         if (src < 0 || src >= numNodes || dest < 0 || dest >= numNodes || weight < 0) {
//             fprintf(stderr, "Invalid edge data: src=%d, dest=%d, weight=%d.\n", src, dest, weight);
//             freeGraph(graph);
//             fclose(file);
//             return EXIT_FAILURE;
//         }
//         addEdge(graph, src, dest, weight);
//     }
//
//     int startNodeDijkstra, endNodeDijkstra;
//     if (fscanf(file, "%d %d", &startNodeDijkstra, &endNodeDijkstra) != 2) {
//         fprintf(stderr, "Error reading start and end nodes for Dijkstra.\n");
//         freeGraph(graph);
//         fclose(file);
//         return EXIT_FAILURE;
//     }
//
//     if (startNodeDijkstra < 0 || startNodeDijkstra >= numNodes || endNodeDijkstra < 0 || endNodeDijkstra >= numNodes) {
//         fprintf(stderr, "Invalid start or end node: start=%d, end=%d.\n", startNodeDijkstra, endNodeDijkstra);
//         freeGraph(graph);
//         fclose(file);
//         return EXIT_FAILURE;
//     }
//
//     fclose(file);
//
//     calculateNodePositions(graph);
//     dijkstra(graph, startNodeDijkstra, endNodeDijkstra);
//
//     // Initialize entity position
//     if (shortestPathLength > 0) {
//         entityPosition = graph->adjList[shortestPath[0]].position;
//         startNodePos = graph->adjList[shortestPath[0]].position;
//         endNodePos = graph->adjList[shortestPath[0]].position;
//     }
//
//     // Main game loop
//     while (!WindowShouldClose()) {
//         // Update
//         //----------------------------------------------------------------------------------
//        if (animationRunning && shortestPathLength > 1) {
//
//     float delta = GetFrameTime();
//
//     if (currentPathIndex >= shortestPathLength - 1) {
//         animationRunning = false;
//     }
//
//     else {
//
//         int src = shortestPath[currentPathIndex];
//         int dst = shortestPath[currentPathIndex + 1];
//
//         Vector2 startPos = graph->adjList[src].position;
//         Vector2 endPos   = graph->adjList[dst].position;
//
//         if (waitingAtNode) {
//             waitTimer += delta;
//
//             if (waitTimer >= NODE_HOLD_TIME) {
//                 waitingAtNode = false;
//                 waitTimer = 0.0f;
//
//                 currentStep = 0;
//
//                 Edge* e = graph->adjList[src].head;
//                 while (e && e->destination != dst) e = e->next;
//
//                 totalSteps = (e ? e->weight : 1);
//                 stepTimer = 0.0f;
//             }
//         }
//
//         else {
//
//             if (totalSteps == 0) {
//                 Edge* e = graph->adjList[src].head;
//                 while (e && e->destination != dst) e = e->next;
//
//                 totalSteps = (e ? e->weight : 1);
//                 currentStep = 0;
//                 stepTimer = 0.0f;
//             }
//
//             stepTimer += delta;
//
//             if (stepTimer >= EDGE_UNIT_TIME) {
//                 stepTimer = 0.0f;
//                 currentStep++;
//
//                 float t = (float)currentStep / totalSteps;
//
//                 entityPosition.x = startPos.x + t * (endPos.x - startPos.x);
//                 entityPosition.y = startPos.y + t * (endPos.y - startPos.y);
//
//                 if (currentStep >= totalSteps) {
//                     entityPosition = endPos;
//
//                     currentPathIndex++;
//                     totalSteps = 0;
//
//                     if (currentPathIndex < shortestPathLength - 1) {
//                         waitingAtNode = true;
//                     }
//                 }
//             }
//         }
//     }
// }
//         if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
//             Vector2 mouse = GetMousePosition();
//
//             if (CheckCollisionPointRec(mouse, (Rectangle){10,10,80,30})) {
//                 animationRunning = !animationRunning;
//             }
//         }
//         if (!animationRunning && currentPathIndex == shortestPathLength - 1) {
//             currentPathIndex = 0;
//             entityPosition = graph->adjList[shortestPath[0]].position;
//             waitingAtNode = false;
//             totalSteps = 0;
//         }
//         //----------------------------------------------------------------------------------
//
//         // Draw
//         //----------------------------------------------------------------------------------
//         BeginDrawing();
//
//         ClearBackground(RAYWHITE);
//
//         DrawGraph(graph);
//
//         EndDrawing();
//         //----------------------------------------------------------------------------------
//     }
//
//     // De-Initialization
//     freeGraph(graph);
//     CloseWindow();

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
