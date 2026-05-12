#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include "raylib.h"
#include "dijkstra.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define NODE_RADIUS 20
#define FONT_SIZE 15
#define ENTITY_RADIUS 10
#define EDGE_UNIT_TIME 0.3f
#define MAX_TRAVELERS 10

// Manual Vector2 Helper Functions
Vector2 MyVector2Sub(Vector2 v1, Vector2 v2) { return (Vector2){ v1.x - v2.x, v1.y - v2.y }; }
Vector2 MyVector2Add(Vector2 v1, Vector2 v2) { return (Vector2){ v1.x + v2.x, v1.y + v2.y }; }
Vector2 MyVector2Scale(Vector2 v, float scale) { return (Vector2){ v.x * scale, v.y * scale }; }
float MyVector2Len(Vector2 v) { return sqrtf(v.x * v.x + v.y * v.y); }
Vector2 MyVector2Norm(Vector2 v) {
    float length = MyVector2Len(v);
    if (length > 0) return (Vector2){ v.x / length, v.y / length };
    return (Vector2){ 0, 0 };
}
Vector2 MyVector2Lrp(Vector2 v1, Vector2 v2, float amount) {
    return (Vector2){ v1.x + amount * (v2.x - v1.x), v1.y + amount * (v2.y - v1.y) };
}

typedef struct {
    Vector2 position;
} NodeGUI;

typedef struct Traveler {
    pid_t pid;
    int startNode;
    int endNode;
    int path[MAX_NODES];
    int pathLength;
    Color color;
    bool active;
    int currentPathIndex;
    Vector2 entityPosition;
    Vector2 startNodePos;
    Vector2 endNodePos;
    float travelTime;
    float elapsedTravelTime;
} Traveler;

Traveler travelers[MAX_TRAVELERS];
int numTravelers = 0;
NodeGUI nodePositions[MAX_NODES];
bool animationRunning = false;

void calculateNodePositions(int numNodes) {
    float angle_step = 2 * PI / numNodes;
    float radius = (SCREEN_HEIGHT < SCREEN_WIDTH ? SCREEN_HEIGHT : SCREEN_WIDTH) / 2 - NODE_RADIUS * 2;
    Vector2 center = (Vector2){SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};
    for (int i = 0; i < numNodes; i++) {
        nodePositions[i].position.x = center.x + radius * cos(i * angle_step);
        nodePositions[i].position.y = center.y + radius * sin(i * angle_step);
    }
}

void DrawGraph(Graph* graph) {
    for (int i = 0; i < graph->numNodes; i++) {
        Vector2 startPos = nodePositions[i].position;
        Edge* curr = graph->adjList[i].head;
        while (curr) {
            Vector2 endPos = nodePositions[curr->destination].position;
            DrawLineEx(startPos, endPos, 2, GRAY);
            Vector2 diff = MyVector2Sub(endPos, startPos);
            Vector2 normDiff = MyVector2Norm(diff);
            Vector2 arrowBase = MyVector2Sub(endPos, MyVector2Scale(normDiff, NODE_RADIUS + 5));
            float angle = atan2f(normDiff.y, normDiff.x);
            DrawTriangle(arrowBase, MyVector2Add(arrowBase, (Vector2){-10 * cosf(angle - PI/6), -10 * sinf(angle - PI/6)}), MyVector2Add(arrowBase, (Vector2){-10 * cosf(angle + PI/6), -10 * sinf(angle + PI/6)}), GRAY);
            char weightText[10]; sprintf(weightText, "%d", curr->weight);
            DrawText(weightText, MyVector2Scale(MyVector2Add(startPos, endPos), 0.5f).x + 5, MyVector2Scale(MyVector2Add(startPos, endPos), 0.5f).y - 15, FONT_SIZE, BLACK);
            curr = curr->next;
        }
    }
    for (int i = 0; i < graph->numNodes; i++) {
        DrawCircleV(nodePositions[i].position, NODE_RADIUS, LIGHTGRAY);
        DrawCircleLines(nodePositions[i].position.x, nodePositions[i].position.y, NODE_RADIUS, DARKGRAY);
        char nodeText[5]; sprintf(nodeText, "%d", i);
        DrawText(nodeText, nodePositions[i].position.x - MeasureText(nodeText, FONT_SIZE) / 2, nodePositions[i].position.y - FONT_SIZE / 2, FONT_SIZE, BLACK);
    }
    for (int i = 0; i < numTravelers; i++) {
        if (travelers[i].active && (animationRunning || (travelers[i].pathLength > 0 && travelers[i].currentPathIndex == travelers[i].pathLength - 1))) {
            if (travelers[i].pathLength > 0) {
                for (int j = 0; j < travelers[i].pathLength - 1; j++)
                    DrawLineEx(nodePositions[travelers[i].path[j]].position, nodePositions[travelers[i].path[j+1]].position, 4, travelers[i].color);
            }
            DrawCircleV(travelers[i].entityPosition, ENTITY_RADIUS, travelers[i].color);
        }
    }
    DrawRectangle(10, 10, 80, 30, animationRunning ? RED : GREEN);
    DrawText(animationRunning ? "STOP" : "PLAY", 20, 18, 20, WHITE);
}

int main(int argc, char* argv[]) {
    if (argc < 2) { fprintf(stderr, "Usage: %s <input_file>\n", argv[0]); return 1; }
    FILE* file = fopen(argv[1], "r");
    int numNodes, numEdges; fscanf(file, "%d %d", &numNodes, &numEdges);
    Graph* graph = createGraph(numNodes);
    for (int i = 0; i < numEdges; i++) { int u, v, w; fscanf(file, "%d %d %d", &u, &v, &w); addEdge(graph, u, v, w); }
    calculateNodePositions(numNodes);
    fscanf(file, "%d", &numTravelers);
    Color colors[] = {BLUE, GREEN, YELLOW, MAGENTA, ORANGE, LIME, SKYBLUE, VIOLET, BEIGE, BROWN};
    for (int i = 0; i < numTravelers; i++) {
        fscanf(file, "%d %d", &travelers[i].startNode, &travelers[i].endNode);
        travelers[i].color = colors[i % 10]; travelers[i].active = true;
        int dummyWeight;
        dijkstra(graph, travelers[i].startNode, travelers[i].endNode, travelers[i].path, &travelers[i].pathLength, &dummyWeight);
        travelers[i].entityPosition = nodePositions[travelers[i].startNode].position;
        pid_t pid = fork();
        if (pid == 0) { printf("[PID=%d] started\n", getpid()); sleep(5); printf("[PID=%d] finished\n", getpid()); exit(0); }
        else { travelers[i].pid = pid; }
    }
    fclose(file);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Graph Visualization - Milestone 4");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), (Rectangle){10, 10, 80, 30})) {
            animationRunning = !animationRunning;
            if (animationRunning) {
                for (int i = 0; i < numTravelers; i++) {
                    travelers[i].currentPathIndex = 0; travelers[i].travelTime = 0; travelers[i].elapsedTravelTime = 0;
                    travelers[i].entityPosition = nodePositions[travelers[i].startNode].position;
                }
            }
        }
        if (animationRunning) {
            for (int i = 0; i < numTravelers; i++) {
                if (travelers[i].currentPathIndex < travelers[i].pathLength - 1) {
                    if (travelers[i].travelTime == 0) {
                        int u = travelers[i].path[travelers[i].currentPathIndex], v = travelers[i].path[travelers[i].currentPathIndex+1], w = 0;
                        Edge* e = graph->adjList[u].head; while(e) { if(e->destination == v) { w = e->weight; break; } e = e->next; }
                        travelers[i].travelTime = w * EDGE_UNIT_TIME; travelers[i].startNodePos = nodePositions[u].position; travelers[i].endNodePos = nodePositions[v].position;
                    }
                    travelers[i].elapsedTravelTime += GetFrameTime();
                    if (travelers[i].elapsedTravelTime < travelers[i].travelTime) travelers[i].entityPosition = MyVector2Lrp(travelers[i].startNodePos, travelers[i].endNodePos, travelers[i].elapsedTravelTime / travelers[i].travelTime);
                    else { travelers[i].entityPosition = travelers[i].endNodePos; travelers[i].travelTime = 0; travelers[i].currentPathIndex++; }
                }
            }
        }
        BeginDrawing(); ClearBackground(RAYWHITE); DrawGraph(graph); EndDrawing();
    }
    for (int i = 0; i < numTravelers; i++) waitpid(travelers[i].pid, NULL, 0);
    freeGraph(graph); CloseWindow(); return 0;
}