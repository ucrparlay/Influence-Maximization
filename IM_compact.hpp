#pragma once

#include <optional>
#include <unordered_set>

#include "get_time.hpp"
#include "graph.hpp"
#include "parlay/sequence.h"
#include "union_find.hpp"
using namespace std;
using namespace parlay;

class CompactInfluenceMaximizer {
 private:
  size_t n, m, R, center_cnt;
  Graph& G;
  float compact_rate;
  sequence<Hash_Edge> hash_edges;
  sequence<bool> is_center, is_seed;
  sequence<size_t> center_id;
  sequence<sequence<size_t>> sketches;

  tuple<optional<NodeId>, bool, size_t> get_center(size_t graph_id, NodeId x);

 public:
  CompactInfluenceMaximizer() = delete;
  CompactInfluenceMaximizer(Graph& graph, float compact_rate, size_t R)
      : R(R), G(graph), compact_rate(compact_rate) {
    assert(G.symmetric);
    n = G.n;
    m = G.m;
    hash_edges = sequence<Hash_Edge>(R);
    parallel_for(0, R, [&](size_t r) {
      hash_edges[r].graph_id = r;
      hash_edges[r].n = G.n;
      hash_edges[r].forward = true;
    });
    is_center = sequence<bool>(n);
    is_seed = sequence<bool>(n);
    center_id = sequence<size_t>(n);
  }
  void init_sketches();
  sequence<pair<NodeId, float>> select_seeds(int k);
};

// space: O(n / center_cnt)
tuple<optional<NodeId>, bool, size_t> CompactInfluenceMaximizer::get_center(
    size_t graph_id, NodeId x) {
  optional<NodeId> center = {};
  if (is_center[x]) {
    return {x, false, 1};
  }
  bool meet_seed = is_seed[x];
  vector<NodeId> que = {x};
  unordered_set<NodeId> visit;
  visit.insert(x);
  const auto& hash_edge = hash_edges[graph_id];
  for (unsigned int i = 0; i < que.size(); i++) {
    if (center.has_value()) break;
    auto u = que[i];
    for (auto j = G.offset[u]; j < G.offset[u + 1]; j++) {
      auto v = G.E[j];
      if ((u < v ? hash_edge(u, v, G.W[j]) : hash_edge(v, u, G.W[j])) &&
          visit.find(v) == visit.end()) {
        que.push_back(v);
        visit.insert(v);
        if (is_seed[v]) {
          meet_seed = true;
        }
        if (is_center[v]) {
          center = v;
          break;
        }
      }
    }
  }
  return {center, meet_seed, visit.size()};
}

void CompactInfluenceMaximizer::init_sketches() {
  timer tt;

  parallel_for(0, n, [&](int i) {
    Hash_Edge hash_edge;  // use hash_edge as random generator
    hash_edge.graph_id = i + R;
    hash_edge.n = G.n;
    hash_edge.forward = true;
    if (hash_edge(i, i, compact_rate)) {
      is_center[i] = true;
      center_id[i] = 1;
    }
  });
  center_cnt = parlay::scan_inplace(center_id);
  cout << "center_cnt: " << center_cnt << endl;

  sketches = sequence(R, sequence<size_t>(center_cnt));
  auto find = gbbs::find_variants::find_compress;
  auto splice = gbbs::splice_variants::split_atomic_one;
  auto unite =
      gbbs::unite_variants::UniteRemCAS<decltype(splice), decltype(find),
                                        find_atomic_halve>(find, splice);
  sequence<sequence<NodeId>> labels(R, sequence<NodeId>(center_cnt));
  parallel_for(0, R, [&](size_t r) {
    parallel_for(0, center_cnt, [&](size_t i) { labels[r][i] = i; });
  });
  sequence<NodeId> belong(n);
  for (size_t r = 0; r < R; r++) {
    parallel_for(0, n, [&](size_t u) {
      auto u_center = std::get<0>(get_center(r, u));
      if (u_center.has_value()) {
        belong[u] = center_id[u_center.value()];
      } else {
        belong[u] = center_cnt;  // center not found
      }
    });
    parallel_for(0, n, [&](size_t u) {
      for (auto j = G.offset[u]; j < G.offset[u + 1]; j++) {
        auto v = G.E[j];
        if (u < v && hash_edges[r](u, v, G.W[j])) {
          if (belong[u] < center_cnt && belong[v] < center_cnt) {
            if (belong[u] != belong[v]) {
              unite(belong[u], belong[v], labels[r]);
            }
          }
        }
      }
    });
    parallel_for(0, center_cnt,
                 [&](size_t i) { sketches[r][i] = find(i, labels[r]); });
    parallel_for(0, n, [&](size_t i) {
      if (belong[i] < center_cnt) {
        belong[i] = sketches[r][belong[i]];
      }
    });
    auto hist = histogram_by_index(belong, center_cnt + 1);
    parallel_for(0, center_cnt, [&](size_t i) {
      if (hist[i] != 0) {
        sketches[r][i] = TOP_BIT | (size_t)hist[i];
      }
    });
  }

  cout << "init_sketches time: " << tt.stop() << endl;
}

sequence<pair<NodeId, float>> CompactInfluenceMaximizer::select_seeds(int k) {
  timer tt;
  sequence<pair<NodeId, float>> seeds(k);
  sequence<size_t> influence(n);
  for (int round = 0; round < k; round++) {
    size_t max_influence = 0;
    parallel_for(0, n, [&](size_t i) {
      size_t new_influence = 0;
      if (round == 0 || influence[i] >= max_influence) {
        for (size_t r = 0; r < R; r++) {
          auto [center, meet_seed, num] = get_center(r, i);
          if (center.has_value()) {
            auto c = center_id[center.value()];
            auto p = sketches[r][c];
            if (!(p & TOP_BIT)) {
              p = sketches[r][p];
            }
            new_influence += p & VAL_MASK;
          } else {
            if (!meet_seed) {
              new_influence += num;
            }
          }
        }
        influence[i] = new_influence;
      }
      if (new_influence > max_influence) {
        write_max(&max_influence, new_influence,
                  [](size_t a, size_t b) { return a < b; });
      }
    });
    size_t seed = parlay::max_element(influence) - influence.begin();
    float influence_gain = max_influence / (R + 0.0);
    seeds[round] = {seed, influence_gain};
    influence[seed] = 0;
    is_seed[seed] = true;
    parallel_for(0, R, [&](size_t r) {
      auto center = std::get<0>(get_center(r, seed));
      if (center.has_value()) {
        auto c = center_id[center.value()];
        auto p = sketches[r][c];
        if (!(p & TOP_BIT)) {
          sketches[r][p] = TOP_BIT | 0;
        } else {
          sketches[r][c] = TOP_BIT | 0;
        }
      }
    });
  }
  cout << "select_seeds time: " << tt.stop() << endl;
  return seeds;
}
