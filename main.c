#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>

#define MAX_NODES 15 // حسب طلب الدكتور: 15 عقدة كحد أقصى
#define INF INT_MAX

int main() {
    // 1. فتح ملف القراءة
    FILE *file = fopen("input.txt", "r");
    if (file == NULL) {
        printf("Error: Could not open input.txt\n");
        return 1;
    }

    int N, M;
    if (fscanf(file, "%d %d", &N, &M) != 2) {
        fclose(file);
        return 0;
    }

    // 2. تمثيل الجراف (مصفوفة تجاور)
    int adj[MAX_NODES][MAX_NODES];
    for (int i = 0; i < MAX_NODES; i++) {
        for (int j = 0; j < MAX_NODES; j++) {
            adj[i][j] = INF; // تعبئة المصفوفة بمسافات لا نهائية كبداية
        }
    }

    // قراءة الروابط والأوزان
    for (int i = 0; i < M; i++) {
        int u, v, w;
        fscanf(file, "%d %d %d", &u, &v, &w);
        adj[u][v] = w;
    }

    int startNode, endNode;
    fscanf(file, "%d %d", &startNode, &endNode);
    fclose(file);

    // حالة إذا كانت البداية هي نفسها النهاية
    if (startNode == endNode) {
        printf("%d\n0\n", startNode);
        return 0;
    }

    // 3. خوارزمية دايكسترا (Dijkstra)
    int dist[MAX_NODES];
    int parent[MAX_NODES];
    bool visited[MAX_NODES];

    for (int i = 0; i < N; i++) {
        dist[i] = INF;
        parent[i] = -1;
        visited[i] = false;
    }

    dist[startNode] = 0;

    for (int count = 0; count < N - 1; count++) {
        // البحث عن العقدة صاحبة أقل مسافة
        int min = INF, u = -1;
        for (int v = 0; v < N; v++) {
            if (!visited[v] && dist[v] <= min) {
                min = dist[v];
                u = v;
            }
        }

        if (u == -1 || min == INF) break;
        visited[u] = true;

        // تحديث مسافات الجيران
        for (int v = 0; v < N; v++) {
            if (!visited[v] && adj[u][v] != INF && dist[u] != INF && dist[u] + adj[u][v] < dist[v]) {
                dist[v] = dist[u] + adj[u][v];
                parent[v] = u;
            }
        }
    }

    // 4. طباعة النتائج
    if (dist[endNode] == INF) {
        printf("No path found\n");
    } else {
        int path[MAX_NODES];
        int path_len = 0;
        
        // تتبع المسار بالعكس
        for (int at = endNode; at != -1; at = parent[at]) {
            path[path_len++] = at;
        }

        // طباعة المسار من البداية للنهاية
        for (int i = path_len - 1; i >= 0; i--) {
            printf("%d", path[i]);
            if (i > 0) printf("->");
        }
        printf("\n%d\n", dist[endNode]);
    }

    return 0;
}