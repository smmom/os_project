#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/mman.h>
#include "raylib.h"
#include "dijkstra.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define NODE_RADIUS 20
#define FONT_SIZE 15
#define ENTITY_RADIUS 10
#define MAX_TRAVELERS 10

typedef struct {
    pid_t pid;
    int arrivedAt;
    int nextNode;
    bool finished;
} IPCMessage;

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
Vector2 MyVector2Lerp(Vector2 v1, Vector2 v2, float amount) {
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
    Vector2 entityPosition;

    // Animation state
    int currentFromNode;
    int currentToNode;
    float moveProgress; // 0.0 to 1.0
    float moveDuration; // Seconds
} Traveler;

Traveler travelers[MAX_TRAVELERS];
int numTravelers = 0;
NodeGUI nodePositions[MAX_NODES];
enum { STATE_IDLE, STATE_RUNNING, STATE_PAUSED } animationState = STATE_IDLE;
int pipefd[2];

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
        if (travelers[i].active) {
            if (travelers[i].pathLength > 0) {
                for (int j = 0; j < travelers[i].pathLength - 1; j++)
                    DrawLineEx(nodePositions[travelers[i].path[j]].position, nodePositions[travelers[i].path[j+1]].position, 4, travelers[i].color);
            }
            DrawCircleV(travelers[i].entityPosition, ENTITY_RADIUS, travelers[i].color);
        }
    }

    Color btnColor = GREEN;
    char* btnText = "PLAY";
    if (animationState == STATE_RUNNING) { btnColor = RED; btnText = "PAUSE"; }
    else if (animationState == STATE_PAUSED) { btnColor = GREEN; btnText = "PLAY"; }

    DrawRectangle(10, 10, 100, 30, btnColor);
    DrawText(btnText, 10 + (100 - MeasureText(btnText, 20)) / 2, 15, 20, WHITE);
}

void StartTravelers(Graph* graph) {
    for (int i = 0; i < numTravelers; i++) {
        pid_t pid = fork();

       if (pid < 0) {
          perror("fork");
          continue;
}

       if (pid == 0) { //child process 
            close(pipefd[0]);
            int p[MAX_NODES], len, w;
            if (dijkstra(graph, travelers[i].startNode, travelers[i].endNode, p, &len, &w)) {
                for (int j = 0; j < len; j++) {
                    IPCMessage m = { getpid(), p[j], (j < len - 1) ? p[j+1] : -1, (j == len - 1) };
                    write(pipefd[1], &m, sizeof(IPCMessage));
                    if (j < len - 1) {
                        int weight = 0;
                        Edge* e = graph->adjList[p[j]].head;
                        while (e) { 
                            if (e->destination == p[j+1]) { 
                                weight = e->weight; break; 
                            }
                            e = e->next; 
                        }
                        usleep(weight * 300000);
                    }
                }
            }
            IPCMessage f = { getpid(), -1, -1, true };
            write(pipefd[1], &f, sizeof(IPCMessage));
            close(pipefd[1]); 
            exit(0);
        } else { 
           travelers[i].pid = pid;
       }
    }
}

void PauseTravelers() {
    for (int i = 0; i < numTravelers; i++)
        if (travelers[i].pid > 0) kill(travelers[i].pid, SIGSTOP);
}

void ResumeTravelers() {
    for (int i = 0; i < numTravelers; i++)
        if (travelers[i].pid > 0) kill(travelers[i].pid, SIGCONT);
}

void KillAllTravelers() {
    for (int i = 0; i < numTravelers; i++) {
        if (travelers[i].pid > 0) {
            kill(travelers[i].pid, SIGKILL);
            waitpid(travelers[i].pid, NULL, 0);
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) { fprintf(stderr, "Usage: %s <input_file>\n", argv[0]); return 1; }
    FILE* file = fopen(argv[1], "r");
if (!file) {
    perror("Error opening input file");
    return 1;
}

int numNodes, numEdges;
if (fscanf(file, "%d %d", &numNodes, &numEdges) != 2) {
    fprintf(stderr, "Invalid graph format\n");
    fclose(file);
    return 1;
}

if (numNodes <= 0 || numNodes > MAX_NODES) {
    fprintf(stderr, "Invalid number of nodes\n");
    fclose(file);
    return 1;
}
    Graph* graph = createGraph(numNodes);
for (int i = 0; i < numEdges; i++) {
    int u, v, w;

    if (fscanf(file, "%d %d %d", &u, &v, &w) != 3) {
        fprintf(stderr, "Invalid edge format\n");
        freeGraph(graph);
        fclose(file);
        return 1;
    }

    if (!addEdge(graph, u, v, w)) {
        fprintf(stderr, "Invalid edge values\n");
        freeGraph(graph);
        fclose(file);
        return 1;
    }
}  
    calculateNodePositions(numNodes);
    if (fscanf(file, "%d", &numTravelers) != 1) {
    fprintf(stderr, "Invalid travelers count\n");
    freeGraph(graph);
    fclose(file);
    return 1;
}

if (numTravelers < 0 || numTravelers > MAX_TRAVELERS) {
    fprintf(stderr, "Invalid number of travelers\n");
    freeGraph(graph);
    fclose(file);
    return 1;
}
    Color colors[] = {BLUE, GREEN, YELLOW, MAGENTA, ORANGE, LIME, SKYBLUE, VIOLET, BEIGE, BROWN};
    for (int i = 0; i < numTravelers; i++) {
        if (fscanf(file, "%d %d", &travelers[i].startNode, &travelers[i].endNode) != 2) {
    fprintf(stderr, "Invalid traveler format\n");
    freeGraph(graph);
    fclose(file);
    return 1;
}

if (travelers[i].startNode < 0 || travelers[i].endNode < 0 ||
    travelers[i].startNode >= numNodes || travelers[i].endNode >= numNodes) {
    fprintf(stderr, "Invalid traveler source or destination\n");
    freeGraph(graph);
    fclose(file);
    return 1;
}
        travelers[i].color = colors[i % 10]; travelers[i].active = true;
        travelers[i].entityPosition = nodePositions[travelers[i].startNode].position;
        travelers[i].pid = 0; travelers[i].pathLength = 0;
        travelers[i].currentFromNode = travelers[i].startNode;
        travelers[i].currentToNode = travelers[i].startNode;
        travelers[i].moveProgress = 1.0f;
    }
    fclose(file);
    if (pipe(pipefd) == -1) {
    perror("pipe");
    freeGraph(graph);
    return 1;
}

if (fcntl(pipefd[0], F_SETFL, O_NONBLOCK) == -1) {
    perror("fcntl");
    close(pipefd[0]);
    close(pipefd[1]);
    freeGraph(graph);
    return 1;
}

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Graph Visualization - Milestone 5 ");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), (Rectangle){10, 10, 100, 30})) {
            if (animationState == STATE_IDLE) { animationState = STATE_RUNNING; StartTravelers(graph); }
            else if (animationState == STATE_RUNNING) { animationState = STATE_PAUSED; PauseTravelers(); }
            else if (animationState == STATE_PAUSED) { animationState = STATE_RUNNING; ResumeTravelers(); }
        }

        if (animationState == STATE_RUNNING) {
            IPCMessage m;
            while (read(pipefd[0], &m, sizeof(IPCMessage)) > 0) {
                for (int i = 0; i < numTravelers; i++) {
                    if (travelers[i].pid == m.pid) {
                        if (m.finished && m.arrivedAt == -1) { travelers[i].active = false; printf("[PID=%d] finished\n", m.pid); }
                        else {
                            if (m.nextNode == -1) printf("[PID=%d] arrived at node %d | DESTINATION\n", m.pid, m.arrivedAt);
                            else printf("[PID=%d] arrived at node %d | next node: %d\n", m.pid, m.arrivedAt, m.nextNode);

                            if (travelers[i].pathLength == 0) { int dw; dijkstra(graph, travelers[i].startNode, travelers[i].endNode, travelers[i].path, &travelers[i].pathLength, &dw); }

                            // Start smooth move to next node
                            if (m.nextNode != -1) {
                                travelers[i].currentFromNode = m.arrivedAt;
                                travelers[i].currentToNode = m.nextNode;
                                travelers[i].moveProgress = 0.0f;
                                // Calculate duration based on edge weight
                                int weight = 0; Edge* e = graph->adjList[m.arrivedAt].head;
                                while (e) { if (e->destination == m.nextNode) { weight = e->weight; break; } e = e->next; }
                                travelers[i].moveDuration = weight * 0.3f; // Same as usleep factor
                            } else {
                                travelers[i].entityPosition = nodePositions[m.arrivedAt].position;
                                travelers[i].moveProgress = 1.0f;
                            }
                        }
                    }
                }
            }

            // Update smooth positions
            float dt = GetFrameTime();
            for (int i = 0; i < numTravelers; i++) {
                if (travelers[i].active && travelers[i].moveProgress < 1.0f) {
                    travelers[i].moveProgress += dt / travelers[i].moveDuration;
                    if (travelers[i].moveProgress > 1.0f) travelers[i].moveProgress = 1.0f;
                    travelers[i].entityPosition = MyVector2Lerp(
                        nodePositions[travelers[i].currentFromNode].position,
                        nodePositions[travelers[i].currentToNode].position,
                        travelers[i].moveProgress
                    );
                }
            }
        }
        BeginDrawing(); ClearBackground(RAYWHITE); DrawGraph(graph); EndDrawing();
    }
    KillAllTravelers(); freeGraph(graph); CloseWindow(); return 0;
}
