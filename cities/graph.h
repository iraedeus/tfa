#ifndef GRAPH_H
#define GRAPH_H

#include <queue>
#include <vector>

struct Edge {
  int u;
  int v;
  double weight;
};

struct GraphNode {
  int to;
  double weight;
};

// Структура для приоритетной очереди расширения государства
struct Candidate {
  double weight;
  int to;

  bool operator>(const Candidate &other) const { return weight > other.weight; }
};

struct SimulationResult {
  std::vector<int> city_to_state; // Индекс государства для каждого города
  std::vector<std::vector<int>>
      states; // Список городов для каждого государства
};

SimulationResult partition_cities(int n, const std::vector<Edge> &edges,
                                  const std::vector<int> &capitals);

#endif // GRAPH_H
