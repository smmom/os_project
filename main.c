// #include <stdio.h>
// #include <stdlib.h>
// #include <limits.h>
// #include <stdbool.h>
//
//
// #define MAX_NODES 15 // As requested in the project instructions: max 15 nodes
// #define INF INT_MAX
// // int *dist[MAX_NODES];
// // int parent[MAX_NODES];
// // bool visited[MAX_NODES];
// // each pint of the graph and the wight of it
// typedef struct Edge {
//     int to;
//     int weight;
//     struct Edge *next;
// }Edge;
// // group of edge make a graph
// typedef struct {
//     Edge **first;
//     int vertices; //pints
// }Graph;
//
// Graph *create_graph(int vertices) {
//     Graph *g = malloc(sizeof(Graph));
//     if (!g) return NULL;
//
//     g->vertices = vertices;
//     g->first = calloc(vertices, sizeof(Edge *));
//     if (!g->first) {
//         free(g);
//         return NULL;
//     }
//
//     return g;
// }
// bool add_edge(Graph *g, int src, int dst, int weight) {
//     Edge *e = malloc(sizeof(Edge));
//     if (!e) return false;
//
//     e->to = dst;
//     e->weight = weight;
//     e->next = g->first[src];
//     g->first[src] = e;
//
//     return true;
// }
//
//
// static void free_graph(Graph *graph) {
//     if (graph == NULL || graph->first == NULL) {
//         return;
//     }
//     for (int i = 0; i < graph->vertices; i++) { Edge *current = graph->first[i];
//         while (current != NULL) { Edge *next = current->next; free(current); current = next; }
//     }
//     free(graph->first);
//     graph->first = NULL;
//     graph->vertices = 0;
// }
// // 3. Dijkstra's Algorithm Implementation
// static void run_dijkstra(const Graph *graph, int source,int  *dist,  int *parent) {
//     bool *visited = calloc(graph->vertices, sizeof(bool));
//     // Initialize distances, parents, and visited arrays
//     for (int i = 0; i < graph->vertices; i++) {
//         dist[i] = INF;
//         parent[i] = -1;
//         // visited[i] = false;
//     }
//     dist[source] = 0;
//     // Find shortest path for all nodes
//     for (int i = 0; i < graph->vertices; i++) {
//             // Find the unvisited node with the minimum distance
//             int u = -1;
//             int min = INF ;
//         // pick unvisited minimum distance node
//             for (int v = 0; v < graph->vertices; v++) {
//                 if (!visited[v] && dist[v] < min) {
//                     min = dist[v];
//                     u = v;
//                 }
//             }
//             // If all remaining nodes are unreachable, break
//             if (u == -1 || min ==INF) break;
//             visited[u] = true;
//             // Update the distances of the adjacent nodes
//             for (Edge *e = graph->first[u]; e; e = e->next) {
//                 int v = e->to;
//
//                 if (!visited[v] &&
//                     dist[u] != INF &&
//                     dist[u] + e->weight <= dist[v]) {
//
//                     dist[v] = dist[u] + e->weight;
//                     parent[v] = u;
//                     }
//             }
//         }
//
//     free(visited);
// }
// void print_path(int endNode, int *dist, int *parent) {
//     // 4. Print the final results in the required format
//     if (dist[endNode] == INF) {
//         // Graph is not connected or no path exists
//         printf("No path found\n");
//         return;
//     }
//         int path[MAX_NODES];
//         int path_len = 0;
//
//         // Backtrack from the end node to find the path
//         for (int at = endNode; at != -1; at = parent[at]) {
//             path[path_len++] = at;
//         }
//
//         // Print the path from start to end with "->" separator
//         for (int i = path_len - 1; i >= 0; i--) {
//             printf("%d", path[i]);
//             if (i > 0) printf("->");
//         }
//
//         // Print the total weight of the shortest path
//         printf("\n%d\n", dist[endNode]);
//
// }
//
//
//
// int main( int argc, char **argv) {
//     if (argc != 2) {
//         printf("Usage: %s <input_file>\n", argv[0]);
//         return 1;
//     }
//     // 1. Open the input file for reading
//     FILE *file = fopen(argv[1], "r");
//     if (file == NULL) {
//         printf("Error: Could not open input.txt\n");
//         return 1;
//     }
//
//     int N, M;
//     // Read the number of nodes (N) and edges (M)
//     if (fscanf(file, "%d %d", &N, &M) != 2) {
//         fclose(file);
//         return 0;
//     }
//     Graph *g = create_graph(N);
//     if (!g) return 1;
//     // Read edges and weights from the file
//     for (int i = 0; i < M; i++) {
//         int u, v, w;
//         fscanf(file, "%d %d %d", &u, &v, &w);
//         add_edge(g, u, v, w);
//     }
//
//
//
//     // 2. Graph representation (Adjacency Matrix)
//     // int adj[MAX_NODES][MAX_NODES];
//     // for (int i = 0; i < MAX_NODES; i++) {
//     //     for (int j = 0; j < MAX_NODES; j++) {
//     //         adj[i][j] = INF; // Initialize the matrix with infinity distances
//     //     }
//     // }
//
//     // Read edges and weights from the file
//     // for (int i  = 0; i < M; i++) {
//     //     int u, v, w;
//     //     fscanf(file, "%d %d %d", &u, &v, &w);
//     //     adj[u][v] = w; // Directed edge from u to v with weight w
//     // }
//
//     int startNode, endNode;
//     fscanf(file, "%d %d", &startNode, &endNode);
//     fclose(file);
//
//     // Handle the edge case where the start node is the same as the end node
//     if (startNode == endNode) {
//         printf("%d\n0\n", startNode);
//         free_graph(g);
//         return 0;
//     }
//     int *dist = malloc(N * sizeof(int));
//     int *parent = malloc(N * sizeof(int));
//
//     run_dijkstra(g,startNode, dist, parent);
//     print_path(endNode, dist, parent);
//
//     free(dist);
//     free(parent);
//     free_graph(g);
//
//     // 3. Dijkstra's Algorithm Implementation
//     // int dist[MAX_NODES];
//     // int parent[MAX_NODES];
//     // bool visited[MAX_NODES];
//     //
//     // // Initialize distances, parents, and visited arrays
//     // for (int i  = 0; i < N; i++) {
//     //     dist[i] = INF;
//     //     parent[i] = -1;
//     //     visited[i] = false;
//     // }
//     //
//     // dist[startNode] = 0;
//     //
//     // // Find shortest path for all nodes
//     // for (int count = 0; count < N - 1; count++) {
//     //     // Find the unvisited node with the minimum distance
//     //     int min = INF, u = -1;
//     //     for (int v = 0; v < N; v++) {
//     //         if (!visited[v] && dist[v] <= min) {
//     //             min = dist[v];
//     //             u = v;
//     //         }
//     //     }
//     //
//     //     // If all remaining nodes are unreachable, break
//     //     if (u == -1 || min == INF) break;
//     //     visited[u] = true;
//     //
//     //     // Update the distances of the adjacent nodes
//     //     for (int v = 0; v < N; v++) {
//     //         if (!visited[v] && adj[u][v] != INF && dist[u] != INF && dist[u] + adj[u][v] < dist[v]) {
//     //             dist[v] = dist[u] + adj[u][v];
//     //             parent[v] = u;
//     //         }
//     //     }
//     // }
//     //
//     // // 4. Print the final results in the required format
//     // if (dist[endNode] == INF) {
//     //     // Graph is not connected or no path exists
//     //     printf("No path found\n");
//     // } else {
//     //     int path[MAX_NODES];
//     //     int path_len = 0;
//     //
//     //     // Backtrack from the end node to find the path
//     //     for (int at = endNode; at != -1; at = parent[at]) {
//     //         path[path_len++] = at;
//     //     }
//     //
//     //     // Print the path from start to end with "->" separator
//     //     for (int i = path_len - 1; i >= 0; i--) {
//     //         printf("%d", path[i]);
//     //         if (i > 0) printf("->");
//     //     }
//     //
//     //     // Print the total weight of the shortest path
//     //     printf("\n%d\n", dist[endNode]);
//     // }
//
//     return 0;
// }
