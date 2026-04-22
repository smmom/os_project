#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>

#define MAX_NODES 15 // As requested in the project instructions: max 15 nodes
#define INF INT_MAX

int main() {
    // 1. Open the input file for reading
    FILE *file = fopen("input.txt", "r");
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

    // 2. Graph representation (Adjacency Matrix)
    int adj[MAX_NODES][MAX_NODES];
    for (int i = 0; i < MAX_NODES; i++) {
        for (int j = 0; j < MAX_NODES; j++) {
            adj[i][j] = INF; // Initialize the matrix with infinity distances
        }
    }

    // Read edges and weights from the file
    for (int i = 0; i < M; i++) {
        int u, v, w;
        fscanf(file, "%d %d %d", &u, &v, &w);
        adj[u][v] = w; // Directed edge from u to v with weight w
    }

    int startNode, endNode;
    fscanf(file, "%d %d", &startNode, &endNode);
    fclose(file);

    // Handle the edge case where the start node is the same as the end node
    if (startNode == endNode) {
        printf("%d\n0\n", startNode);
        return 0;
    }

    // 3. Dijkstra's Algorithm Implementation
    int dist[MAX_NODES];
    int parent[MAX_NODES];
    bool visited[MAX_NODES];

    // Initialize distances, parents, and visited arrays
    for (int i = 0; i < N; i++) {
        dist[i] = INF;
        parent[i] = -1;
        visited[i] = false;
    }

    dist[startNode] = 0;

    // Find shortest path for all nodes
    for (int count = 0; count < N - 1; count++) {
        // Find the unvisited node with the minimum distance
        int min = INF, u = -1;
        for (int v = 0; v < N; v++) {
            if (!visited[v] && dist[v] <= min) {
                min = dist[v];
                u = v;
            }
        }

        // If all remaining nodes are unreachable, break
        if (u == -1 || min == INF) break;
        visited[u] = true;

        // Update the distances of the adjacent nodes
        for (int v = 0; v < N; v++) {
            if (!visited[v] && adj[u][v] != INF && dist[u] != INF && dist[u] + adj[u][v] < dist[v]) {
                dist[v] = dist[u] + adj[u][v];
                parent[v] = u;
            }
        }
    }

    // 4. Print the final results in the required format
    if (dist[endNode] == INF) {
        // Graph is not connected or no path exists
        printf("No path found\n");
    } else {
        int path[MAX_NODES];
        int path_len = 0;
        
        // Backtrack from the end node to find the path
        for (int at = endNode; at != -1; at = parent[at]) {
            path[path_len++] = at;
        }

        // Print the path from start to end with "->" separator
        for (int i = path_len - 1; i >= 0; i--) {
            printf("%d", path[i]);
            if (i > 0) printf("->");
        }
        
        // Print the total weight of the shortest path
        printf("\n%d\n", dist[endNode]);
    }

    return 0;
}