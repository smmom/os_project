#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <stdbool.h>
#include <limits.h>

#define MAX_NODES 15
#define INF INT_MAX

typedef struct Edge {
    int destination;
    int weight;
    struct Edge* next;
} Edge;

typedef struct Node {
    Edge* head;
} Node;

typedef struct Graph {
    int numNodes;
    Node* adjList;
} Graph;

/* Graph */
Graph* createGraph(int numNodes);
bool addEdge(Graph* graph, int src, int dest, int weight);
void freeGraph(Graph* graph);

/* Dijkstra */
bool dijkstra(Graph* graph,int startNode,int endNode,int* path,int* pathLength,int* totalWeight);

#endif
