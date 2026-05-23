#include "graph.h"

SimulationResult partition_cities(int n, const std::vector<Edge> &edges,
                                  const std::vector<int> &capitals) {
  int k = capitals.size();
  std::vector<std::vector<GraphNode>> adj(n);

  for (const auto &edge : edges) {
    adj[edge.u].push_back({edge.v, edge.weight});
    adj[edge.v].push_back({edge.u, edge.weight});
  }

  std::vector<int> city_to_state(n, -1);
  std::vector<std::vector<int>> states(k);

  // Очередь с приоритетом для каждого государства (хранит ближайшие доступные
  // города)
  std::vector<std::priority_queue<Candidate, std::vector<Candidate>,
                                  std::greater<Candidate>>>
      state_pqs(k);

  // Инициализация столиц
  for (int s = 0; s < k; ++s) {
    int cap = capitals[s];
    city_to_state[cap] = s;
    states[s].push_back(cap);

    for (const auto &neighbor : adj[cap]) {
      if (city_to_state[neighbor.to] == -1) {
        state_pqs[s].push({neighbor.weight, neighbor.to});
      }
    }
  }

  int assigned_count = k;
  bool progress = true;

  // Пошаговое расширение по очереди для каждого государства
  while (assigned_count < n && progress) {
    progress = false;
    for (int s = 0; s < k; ++s) {
      while (!state_pqs[s].empty()) {
        Candidate next_city = state_pqs[s].top();
        state_pqs[s].pop();

        // Если город еще не занят другим государством
        if (city_to_state[next_city.to] == -1) {
          city_to_state[next_city.to] = s;
          states[s].push_back(next_city.to);
          assigned_count++;
          progress = true;

          // Добавляем новые доступные дороги из присоединенного города
          for (const auto &neighbor : adj[next_city.to]) {
            if (city_to_state[neighbor.to] == -1) {
              state_pqs[s].push({neighbor.weight, neighbor.to});
            }
          }
          break; // Передаем ход следующему государству
        }
      }
    }
  }

  return {city_to_state, states};
}
