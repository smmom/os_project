#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include "dijkstra.h"


/* ===== Edge ===== */
Edge* createEdge(int dest, int weight) {
    Edge* e = malloc(sizeof(Edge));
    e->destination = dest;
    e->weight = weight;
    e->next = NULL;
    return e;
}

/* ===== Graph ===== */
Graph* createGraph(int numNodes) {
    Graph* graph = malloc(sizeof(Graph));

    graph->numNodes = numNodes;
    graph->adjList = malloc(numNodes * sizeof(Node));

    for (int i = 0; i < numNodes; i++) {
        graph->adjList[i].head = NULL;
    }

    return graph;
}

/* ===== Add Edge ===== */
bool addEdge(Graph *graph, int src, int dest, int weight) {
    if (!graph || src < 0 || dest < 0) return false;

    Edge* e = createEdge(dest, weight);

    if (!graph->adjList[src].head) {
        graph->adjList[src].head = e;
    } else {
        Edge* temp = graph->adjList[src].head;
        while (temp->next) temp = temp->next;
        temp->next = e;
    }

    return true;
}

/* ===== Free Graph ===== */
void freeGraph(Graph *graph) {
    for (int i = 0; i < graph->numNodes; i++) {
        Edge* cur = graph->adjList[i].head;
        while (cur) {
            Edge* tmp = cur;
            cur = cur->next;
            free(tmp);
        }
    }

    free(graph->adjList);
    free(graph);
}

/* ===== Dijkstra ===== */
bool dijkstra(Graph* graph, int startNode, int endNode,
              int* path, int* pathLength, int* totalWeight) {

    int n = graph->numNodes;

    int dist[MAX_NODES];
    int parent[MAX_NODES];
    bool visited[MAX_NODES] = {0};

    for (int i = 0; i < n; i++) {
        dist[i] = INF;
        parent[i] = -1;
    }

    dist[startNode] = 0;

    /* main loop */
    for (int i = 0; i < n; i++) {

        int u = -1;
        int min = INF;

        for (int v = 0; v < n; v++) {
            if (!visited[v] && dist[v] < min) {
                min = dist[v];
                u = v;
            }
        }

        if (u == -1) break;

        visited[u] = true;

        for (Edge* e = graph->adjList[u].head; e; e = e->next) {
            int v = e->destination;

            if (!visited[v] && dist[u] != INF &&
                dist[u] + e->weight < dist[v]) {

                dist[v] = dist[u] + e->weight;
                parent[v] = u;
            }
        }
    }

    if (dist[endNode] == INF) {
        *pathLength = 0;
        *totalWeight = INF;
        return false;
    }

    *totalWeight = dist[endNode];

    /* rebuild path */
    int rev[MAX_NODES];
    int idx = 0;

    for (int at = endNode; at != -1; at = parent[at]) {
        rev[idx++] = at;
    }

    for (int i = 0; i < idx; i++) {
        path[i] = rev[idx - 1 - i];
    }

    *pathLength = idx;

    return true;
}
#ifndef SIMULATION_MODE
int main(int argc, char **argv) {
if (argc != 2) {
    printf("Usage: %s <input_file>\n", argv[0]);
    return 1;
}

FILE *file = fopen(argv[1], "r");
if (!file) {
    printf("Error: cannot open file %s\n", argv[1]);
    return 1;
}

int N, M;

if (fscanf(file, "%d %d", &N, &M) != 2) {
    printf("Invalid input format\n");
    fclose(file);
    return 1;
}

if (N > MAX_NODES) {
    printf("Error: too many nodes (max %d)\n", MAX_NODES);
    fclose(file);
    return 1;
}

Graph* g = createGraph(N);

for (int i = 0; i < M; i++) {
    int u, v, w;

    if (fscanf(file, "%d %d %d", &u, &v, &w) != 3) {
        printf("Invalid edge format\n");
        freeGraph(g);
        fclose(file);
        return 1;
    }

    addEdge(g, u, v, w);
}

int startNode, endNode;
fscanf(file, "%d %d", &startNode, &endNode);

fclose(file);

int path[MAX_NODES];
int pathLen = 0;
int totalWeight = 0;

bool ok = dijkstra(g, startNode, endNode, path, &pathLen, &totalWeight);

if (!ok) {
    printf("No path found\n");
} else {

    for (int i = 0; i < pathLen; i++) {
        printf("%d", path[i]);
        if (i < pathLen - 1)
            printf("->");
    }

    printf("\n%d\n", totalWeight);
}

freeGraph(g);

return 0;
}
#endif
