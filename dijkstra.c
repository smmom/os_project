#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include "dijkstra.h"


#define MAX_NODES 15 // As requested in the project instructions: max 15 nodes
#define INF INT_MAX
// int *dist[MAX_NODES];
// int parent[MAX_NODES];
// bool visited[MAX_NODES];
// each pint of the graph and the wight of it
Edge* createEdge(int dest, int weight) {
    Edge* e = malloc(sizeof(Edge));
    e->destination = dest;
    e->weight = weight;
    e->next = NULL;
    return e;
}

Graph* createGraph(int numNodes) {
    Graph* graph = malloc(sizeof(Graph));
    graph->numNodes = numNodes;
    graph->adjList = malloc(numNodes * sizeof(Node));

    for (int i = 0; i < numNodes; i++) {
        graph->adjList[i].head = NULL;
    }

    return graph;
}

void addEdge(Graph *graph, int src, int dest, int weight) {
    Edge* e = createEdge(dest, weight);

    if (!graph->adjList[src].head) {
        graph->adjList[src].head = e;
    } else {
        Edge* temp = graph->adjList[src].head;
        while (temp->next) temp = temp->next;
        temp->next = e;
    }
}


 void freeGraph(Graph *graph) {

    for (int i = 0; i < graph->numNodes; i++) {
        Edge* current = graph->adjList[i].head;
        while (current) {
            Edge* tmp = current;
            current = current->next;
            free(tmp);
        }
    }
    free(graph->adjList);
        free(graph);

}
// 3. Dijkstra's Algorithm Implementation
bool dijkstra(Graph* graph, int startNode, int endNode,
          int* path, int* pathLength, int* totalWeight) {

        int dist[MAX_NODES];
        int parent[MAX_NODES];
        bool visited[MAX_NODES];

        for (int i = 0; i < graph->numNodes; i++) {
            dist[i] = INT_MAX;
            parent[i] = -1;
            visited[i] = false;
        }

        dist[startNode] = 0;

        for (int i = 0; i < graph->numNodes; i++) {
            int u = -1, min = INF;

            for (int v = 0; v < graph->numNodes; v++) {
                if (!visited[v] && dist[v] < min) {
                    min = dist[v];
                    u = v;
                }
            }

            if (u == -1) break;
            visited[u] = true;

            for (Edge* e = graph->adjList[u].head; e; e = e->next) {
                int v = e->destination;

                if (!visited[v] &&
                    dist[u] != INT_MAX &&
                    dist[u] + e->weight < dist[v]) {

                    dist[v] = dist[u] + e->weight;
                    parent[v] = u;
                    }
            }
        }

        *totalWeight = dist[endNode];

        if (dist[endNode] == INF) {
            *pathLength = 0;
            return false;
        }

        int rev[MAX_NODES];
        int idx = 0;

        for (int at = endNode; at != -1; at = parent[at]) {
            rev[idx++] = at;
        }

        *pathLength = idx;

        for (int i = 0; i < idx; i++) {
            path[i] = rev[idx - 1 - i];
        }
    }
    // Initialize distances, parents, and visited arrays
    // for (int i = 0; i < graph->numNodes; i++) {
    //     dist[i] = INF;
    //     parent[i] = -1;
    //     // visited[i] = false;
    // }
    // dist[source] = 0;
    // // Find shortest path for all nodes
    // for (int i = 0; i < graph->vertices; i++) {
    //         // Find the unvisited node with the minimum distance
    //         int u = -1;
    //         int min = INF ;
    //     // pick unvisited minimum distance node
    //         for (int v = 0; v < graph->vertices; v++) {
    //             if (!visited[v] && dist[v] < min) {
    //                 min = dist[v];
    //                 u = v;
    //             }
    //         }
    //         // If all remaining nodes are unreachable, break
    //         if (u == -1 || min ==INF) break;
    //         visited[u] = true;
    //         // Update the distances of the adjacent nodes
    //         for (Edge *e = graph->first[u]; e; e = e->next) {
    //             int v = e->to;
    //
    //             if (!visited[v] && dist[u] != INF && dist[u] + e->weight <= dist[v]) { // Changed <= to <
    //                 dist[v] = dist[u] + e->weight;
    //                 parent[v] = u;
    //             }
    //         }
    //     }
    //
    // free(visited);
// }
void print_path(int endNode, int *dist, int *parent) {
    // 4. Print the final results in the required format
    if (dist[endNode] == INF) {
        // Graph is not connected or no path exists
        printf("No path found\n");
        return;
    }
        int path[MAX_NODES];
        int path_len = 0;

        // Backtrack from the end node to find the path
        for (int at = endNode; at != -1; at = parent[at]) {
            path[path_len++] = at;
        }

        // Print the path from start to end with "->" separator
        for (int i = path_len - 1; i >= 0; i--) {
            printf("%d", path[i]);
            if (i > 0) printf(" -> ");
        }

        // Print the total weight of the shortest path
        printf("\n%d\n", dist[endNode]);

}

#ifdef DIJKSTRA_MAIN
#include <stdio.h>

int main( int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    // 1. Open the input file for reading
    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        printf("Error: Could not open input.txt\n");
        return 1;
    }

    int N, M;
    // Read the number of nodes (N) and edges (M)
    if (fscanf(file, "%d %d", &N, &M) != 2) {
        fclose(file);
        return 0;
    }
    Graph *g = createGraph(N);
    if (!g) return 1;
    // Read edges and weights from the file
    for (int i = 0; i < M; i++) {
        int u, v, w;
        fscanf(file, "%d %d %d", &u, &v, &w);
        addEdge(g, u, v, w);
    }


    int startNode, endNode;
    fscanf(file, "%d %d", &startNode, &endNode);
    fclose(file);

    // Handle the edge case where the start node is the same as the end node
    if (startNode == endNode) {
        printf("%d\n0\n", startNode);
        freeGraph(g);
        return 0;
    }
    int path[MAX_NODES];
    int pathLen = 0;
    int totalWeight = 0;

    if (!dijkstra(g, startNode, endNode, path, &pathLen, &totalWeight)) {
        printf("No path found\n");
    } else {
        for (int i = 0; i < pathLen; i++) {
            printf("%d", path[i]);
            if (i < pathLen - 1) printf("->");
        }
        printf("\n%d\n", totalWeight);
    }


    freeGraph(g);
    // printf("Running DIJKSTRA file\n");


    return 0;
}
#endif
