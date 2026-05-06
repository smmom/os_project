#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "raylib.h"
#include "raymath.h"

#define MAX_NODES 100
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define NODE_RADIUS 20
#define FONT_SIZE 15
#define ENTITY_RADIUS 10
#define ENTITY_COLOR RED
#define NODE_HOLD_TIME 1.0f // 1 second
#define EDGE_UNIT_TIME 0.3f // 300 milliseconds per unit of weight

// Structure to represent an edge
typedef struct Edge {
    int destination;
    int weight;
    struct Edge* next;
} Edge;

// Structure to represent a node in the adjacency list
typedef struct Node {
    Edge* head;
    Vector2 position; // Position for drawing
} Node;

// Structure to represent the graph
typedef struct Graph {
    int numNodes;
    Node* adjList;
} Graph;

// Global variables for Dijkstra's path and animation
int shortestPath[MAX_NODES];
int shortestPathLength = 0;
int shortestPathTotalWeight = INT_MAX;

// Animation state
bool animationRunning = true;
int currentPathIndex = 0;
float animationTimer = 0.0f;
Vector2 entityPosition;
Vector2 startNodePos;
Vector2 endNodePos;


// Animation state (new improved)
bool waitingAtNode = false;
float waitTimer = 0.0f;

int currentStep = 0;      // current step inside edge
int totalSteps = 0;       // = weight
float stepTimer = 0.0f;   // timer for each step

// Function to create a new edge
Edge* createEdge(int destination, int weight) {
    Edge* newEdge = (Edge*)malloc(sizeof(Edge));
    if (!newEdge) {
        perror("Failed to allocate memory for edge");
        exit(EXIT_FAILURE);
    }
    newEdge->destination = destination;
    newEdge->weight = weight;
    newEdge->next = NULL;
    return newEdge;
}

// Function to create a graph with a given number of nodes
Graph* createGraph(int numNodes) {
    Graph* graph = (Graph*)malloc(sizeof(Graph));
    if (!graph) {
        perror("Failed to allocate memory for graph");
        exit(EXIT_FAILURE);
    }
    graph->numNodes = numNodes;
    graph->adjList = (Node*)malloc(numNodes * sizeof(Node));
    if (!graph->adjList) {
        perror("Failed to allocate memory for adjacency list");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < numNodes; i++) {
        graph->adjList[i].head = NULL;
        graph->adjList[i].position = (Vector2){0, 0}; // Initialize positions
    }
    return graph;
}

// Function to add an edge to the graph
void addEdge(Graph* graph, int src, int dest, int weight) {
    Edge* newEdge = createEdge(dest, weight);
    if (!graph->adjList[src].head) {
        graph->adjList[src].head = newEdge;
    } else {
        Edge* temp = graph->adjList[src].head;
        while (temp->next) temp = temp->next;
        temp->next = newEdge;
    }
}

// Function to free the graph memory
void freeGraph(Graph* graph) {
    for (int i = 0; i < graph->numNodes; i++) {
        Edge* current = graph->adjList[i].head;
        while (current != NULL) {
            Edge* temp = current;
            current = current->next;
            free(temp);
        }
    }
    free(graph->adjList);
    free(graph);
}

// Dijkstra's algorithm implementation (modified to store path)
void dijkstra(Graph* graph, int startNode, int endNode) {
    int distances[MAX_NODES];
    int previous[MAX_NODES];
    bool visited[MAX_NODES];

    for (int i = 0; i < graph->numNodes; i++) {
        distances[i] = INT_MAX;
        previous[i] = -1;
        visited[i] = false;
    }

    distances[startNode] = 0;

    for (int count = 0; count < graph->numNodes; count++) {
        int minDist = INT_MAX;
        int minIndex = -1;

        for (int v = 0; v < graph->numNodes; v++) {
            if (!visited[v] && distances[v] < minDist) {
                minDist = distances[v];
                minIndex = v;
            }
        }

        if (minIndex == -1) break; // No reachable unvisited nodes

        visited[minIndex] = true;

        Edge* currentEdge = graph->adjList[minIndex].head;
        while (currentEdge != NULL) {
            int v = currentEdge->destination;
            int weight = currentEdge->weight;
            if (!visited[v] && distances[minIndex] != INT_MAX &&
                distances[minIndex] + weight < distances[v]) {
                distances[v] = distances[minIndex] + weight;
                previous[v] = minIndex;
            }
            currentEdge = currentEdge->next;
        }
    }

    // Store the path
    shortestPathLength = 0;
    shortestPathTotalWeight = distances[endNode];

    if (shortestPathTotalWeight == INT_MAX) {
        if (shortestPathLength == 0) {
            DrawText("No Path Found", 300, 50, 20, RED);
        }
        // No path found
        return;
    } else if (startNode == endNode) {
        shortestPath[0] = startNode;
        shortestPathLength = 1;
    } else {
        int pathReverse[MAX_NODES];
        int pathReverseIndex = 0;
        int currentNode = endNode;
        while (currentNode != -1) {
            pathReverse[pathReverseIndex++] = currentNode;
            currentNode = previous[currentNode];
        }

        for (int i = 0; i < pathReverseIndex; i++) {
            shortestPath[i] = pathReverse[pathReverseIndex - 1 - i];
        }
        shortestPathLength = pathReverseIndex;
    }
}

// Function to calculate node positions to avoid overlap
void calculateNodePositions(Graph* graph) {
    float angle_step = 2 * PI / graph->numNodes;
    float radius = (SCREEN_HEIGHT < SCREEN_WIDTH ? SCREEN_HEIGHT : SCREEN_WIDTH) / 2 - NODE_RADIUS * 2;
    Vector2 center = (Vector2){SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};

    for (int i = 0; i < graph->numNodes; i++) {
        graph->adjList[i].position.x = center.x + radius * cos(i * angle_step);
        graph->adjList[i].position.y = center.y + radius * sin(i * angle_step);
    }
}

// Main drawing function
void DrawGraph(Graph* graph) {
    // Draw edges first
    for (int i = 0; i < graph->numNodes; i++) {
        Vector2 startPos = graph->adjList[i].position;
        Edge* currentEdge = graph->adjList[i].head;
        while (currentEdge != NULL) {
            Vector2 endPos = graph->adjList[currentEdge->destination].position;

            // Draw arrow line
            DrawLineEx(startPos, endPos, 2, GRAY);

            // Draw arrow head (simple triangle)
            Vector2 diff = Vector2Subtract(endPos, startPos);
            Vector2 normDiff = Vector2Normalize(diff);
            Vector2 arrowBase = Vector2Subtract(endPos, Vector2Scale(normDiff, NODE_RADIUS + 5));

            float angle = atan2f(normDiff.y, normDiff.x);
            Vector2 p1 = arrowBase;
            Vector2 p2 = Vector2Add(arrowBase, (Vector2){-10 * cosf(angle - PI/6), -10 * sinf(angle - PI/6)});
            Vector2 p3 = Vector2Add(arrowBase, (Vector2){-10 * cosf(angle + PI/6), -10 * sinf(angle + PI/6)});
            DrawTriangle(p1, p2, p3, GRAY);

            // Draw weight
            Vector2 midPoint = Vector2Scale(Vector2Add(startPos, endPos), 0.5f);
            char weightText[10];
            sprintf(weightText, "%d", currentEdge->weight);
            DrawText(weightText, midPoint.x + 5, midPoint.y - 15, FONT_SIZE, BLACK);

            currentEdge = currentEdge->next;
        }
    }

    // Draw nodes
    for (int i = 0; i < graph->numNodes; i++) {
        Vector2 pos = graph->adjList[i].position;
        DrawCircleV(pos, NODE_RADIUS, LIGHTGRAY);
        DrawCircleLines(pos.x, pos.y, NODE_RADIUS, DARKGRAY);

        char nodeText[5];
        sprintf(nodeText, "%d", i);
        DrawText(nodeText, pos.x - MeasureText(nodeText, FONT_SIZE) / 2, pos.y - FONT_SIZE / 2, FONT_SIZE, BLACK);
    }

    // Draw shortest path if animation is running or completed
    if (shortestPathLength > 0) {
        for (int i = 0; i < shortestPathLength - 1; i++) {
            Vector2 p1 = graph->adjList[shortestPath[i]].position;
            Vector2 p2 = graph->adjList[shortestPath[i+1]].position;
            DrawLineEx(p1, p2, 4, BLUE);
        }
    }

    // Draw entity
    if (animationRunning || currentPathIndex == shortestPathLength - 1) {
        DrawCircleV(entityPosition, ENTITY_RADIUS, ENTITY_COLOR);
    }

    // Draw play/stop button
    DrawRectangle(10, 10, 80, 30, animationRunning ? RED : GREEN);
    DrawText(animationRunning ? "STOP" : "PLAY", 20, 18, 20, WHITE);

    // Draw arrival message
    if (currentPathIndex == shortestPathLength - 1 && shortestPathLength > 0) {
        DrawText("Arrived at destination!", SCREEN_WIDTH / 2 - MeasureText("Arrived at destination!", 20) / 2, SCREEN_HEIGHT - 30, 20, BLUE);
    }
}

int main(int argc, char* argv[]) {
    // Initialization
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
        return EXIT_FAILURE;
    }

    int numNodes, numEdges;
    if (fscanf(file, "%d %d", &numNodes, &numEdges) != 2) {
        fprintf(stderr, "Error reading number of nodes and edges.\n");
        fclose(file);
        return EXIT_FAILURE;
    }

    if (numNodes > MAX_NODES) {
        fprintf(stderr, "Number of nodes exceeds MAX_NODES (%d).\n", MAX_NODES);
        fclose(file);
        return EXIT_FAILURE;
    }

    Graph* graph = createGraph(numNodes);

    for (int i = 0; i < numEdges; i++) {
        int src, dest, weight;
        if (fscanf(file, "%d %d %d", &src, &dest, &weight) != 3) {
            fprintf(stderr, "Error reading edge data.\n");
            freeGraph(graph);
            fclose(file);
            return EXIT_FAILURE;
        }
        if (src < 0 || src >= numNodes || dest < 0 || dest >= numNodes || weight < 0) {
            fprintf(stderr, "Invalid edge data: src=%d, dest=%d, weight=%d.\n", src, dest, weight);
            freeGraph(graph);
            fclose(file);
            return EXIT_FAILURE;
        }
        addEdge(graph, src, dest, weight);
    }

    int startNodeDijkstra, endNodeDijkstra;
    if (fscanf(file, "%d %d", &startNodeDijkstra, &endNodeDijkstra) != 2) {
        fprintf(stderr, "Error reading start and end nodes for Dijkstra.\n");
        freeGraph(graph);
        fclose(file);
        return EXIT_FAILURE;
    }

    if (startNodeDijkstra < 0 || startNodeDijkstra >= numNodes || endNodeDijkstra < 0 || endNodeDijkstra >= numNodes) {
        fprintf(stderr, "Invalid start or end node: start=%d, end=%d.\n", startNodeDijkstra, endNodeDijkstra);
        freeGraph(graph);
        fclose(file);
        return EXIT_FAILURE;
    }

    fclose(file);

    calculateNodePositions(graph);
    dijkstra(graph, startNodeDijkstra, endNodeDijkstra);

    // Initialize entity position
    if (shortestPathLength > 0) {
        entityPosition = graph->adjList[shortestPath[0]].position;
        startNodePos = graph->adjList[shortestPath[0]].position;
        endNodePos = graph->adjList[shortestPath[0]].position;
    }

    // Main game loop
    while (!WindowShouldClose()) {
        // Update
        //----------------------------------------------------------------------------------
       if (animationRunning && shortestPathLength > 1) {

    float delta = GetFrameTime();

    if (currentPathIndex >= shortestPathLength - 1) {
        animationRunning = false;
    }

    else {

        int src = shortestPath[currentPathIndex];
        int dst = shortestPath[currentPathIndex + 1];

        Vector2 startPos = graph->adjList[src].position;
        Vector2 endPos   = graph->adjList[dst].position;

        if (waitingAtNode) {
            waitTimer += delta;

            if (waitTimer >= NODE_HOLD_TIME) {
                waitingAtNode = false;
                waitTimer = 0.0f;

                currentStep = 0;

                Edge* e = graph->adjList[src].head;
                while (e && e->destination != dst) e = e->next;

                totalSteps = (e ? e->weight : 1);
                stepTimer = 0.0f;
            }
        }

        else {

            if (totalSteps == 0) {
                Edge* e = graph->adjList[src].head;
                while (e && e->destination != dst) e = e->next;

                totalSteps = (e ? e->weight : 1);
                currentStep = 0;
                stepTimer = 0.0f;
            }

            stepTimer += delta;

            if (stepTimer >= EDGE_UNIT_TIME) {
                stepTimer = 0.0f;
                currentStep++;

                float t = (float)currentStep / totalSteps;

                entityPosition.x = startPos.x + t * (endPos.x - startPos.x);
                entityPosition.y = startPos.y + t * (endPos.y - startPos.y);

                if (currentStep >= totalSteps) {
                    entityPosition = endPos;

                    currentPathIndex++;
                    totalSteps = 0;

                    if (currentPathIndex < shortestPathLength - 1) {
                        waitingAtNode = true;
                    }
                }
            }
        }
    }
}
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetMousePosition();

            if (CheckCollisionPointRec(mouse, (Rectangle){10,10,80,30})) {
                animationRunning = !animationRunning;
            }
        }
        if (!animationRunning && currentPathIndex == shortestPathLength - 1) {
            currentPathIndex = 0;
            entityPosition = graph->adjList[shortestPath[0]].position;
            waitingAtNode = false;
            totalSteps = 0;
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawGraph(graph);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    freeGraph(graph);
    CloseWindow();

    return 0;
}
