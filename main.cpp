#include <iostream>
#include <vector>
#include <fstream>
#include <limits>
#include <queue>
#include <algorithm>

using namespace std;

// تعريف هيكل الرابط
struct Edge {
    int to;
    int weight;
};

int main() {
    // 1. قراءة الملف
    ifstream inputFile("input.txt");
    if (!inputFile.is_open()) {
        cerr << "Error: Could not open input.txt" << endl;
        return 1;
    }

    int N, M;
    if (!(inputFile >> N >> M)) return 0;

    vector<vector<Edge>> adj(N);
    for (int i = 0; i < M; ++i) {
        int u, v, w;
        inputFile >> u >> v >> w;
        adj[u].push_back({v, w});
    }

    int startNode, endNode;
    inputFile >> startNode >> endNode;
    inputFile.close();

    // التعامل مع حالة إذا كانت نقطة البداية هي نفسها نقطة النهاية (حسب طلب الـ PDF)
    if (startNode == endNode) {
        cout << startNode << endl;
        cout << "0" << endl;
        return 0;
    }

    // 2. خوارزمية دايكسترا (Dijkstra's Algorithm)
    // مصفوفة لحفظ أقل مسافة لكل عقدة (نبداها بمسافة لا نهائية)
    vector<int> dist(N, numeric_limits<int>::max());
    // مصفوفة لحفظ الآباء عشان نقدر نرسم المسار العكسي
    vector<int> parent(N, -1);
    // طابور أولوية (Priority Queue) لاختيار العقدة صاحبة المسافة الأقل
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;

    dist[startNode] = 0;
    pq.push({0, startNode});

    while (!pq.empty()) {
        int d = pq.top().first;
        int u = pq.top().second;
        pq.pop();

        // إذا لقينا مسار أقصر مسبقاً، نتجاهل
        if (d > dist[u]) continue;

        // المرور على كل جيران العقدة الحالية
        for (const auto& edge : adj[u]) {
            int v = edge.to;
            int weight = edge.weight;

            // تحديث المسافة إذا لقينا مسار أقصر
            if (dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                parent[v] = u;
                pq.push({dist[v], v});
            }
        }
    }

    // 3. طباعة النتائج بالصيغة المطلوبة
    if (dist[endNode] == numeric_limits<int>::max()) {
        // حالة عدم وجود مسار (גרף לא קשיר)
        cout << "No path found" << endl;
    } else {
        // تتبع المسار من النهاية للبداية
        vector<int> path;
        for (int at = endNode; at != -1; at = parent[at]) {
            path.push_back(at);
        }
        // عكس المسار ليكون من البداية للنهاية
        reverse(path.begin(), path.end());

        // طباعة المسار مع أسهم
        for (size_t i = 0; i < path.size(); ++i) {
            cout << path[i];
            if (i < path.size() - 1) cout << "->";
        }
        cout << endl;
        // طباعة الوزن الكلي
        cout << dist[endNode] << endl;
    }

    return 0;
}