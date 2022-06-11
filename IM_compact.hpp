#pragma once

#include <optional>
#include <unordered_set>

#include "concurrent_bitset.hpp"
#include "get_time.hpp"
#include "graph.hpp"
#include "parlay/sequence.h"
#include "union_find.hpp"
using namespace std;
using namespace parlay;

const int BITSET_SIZE = 321065547;  // >= n

class CompactInfluenceMaximizer {
 private:
  size_t n, m, R, center_cnt;
  Graph& G;
  float compact_rate;
  sequence<bool> is_center;
  sequence<size_t> center_id;
  vector<folly::ConcurrentBitSet<BITSET_SIZE>> visit;
  sequence<sequence<size_t>> sketches;

  pair<optional<NodeId>, vector<NodeId>> get_center(size_t graph_id, NodeId x);

 public:
  CompactInfluenceMaximizer() = delete;
  CompactInfluenceMaximizer(Graph& graph, float compact_rate, size_t R)
      : R(R), G(graph), compact_rate(compact_rate) {
    assert(G.symmetric);
    assert(BITSET_SIZE >= graph.n);
    n = G.n;
    m = G.m;
    is_center = sequence<bool>(n);
    center_id = sequence<size_t>(n);
    visit = vector<folly::ConcurrentBitSet<BITSET_SIZE>>(R);
  }
  void init_sketches();
  sequence<pair<NodeId, float>> select_seeds(int k);
};

pair<optional<NodeId>, vector<NodeId>> CompactInfluenceMaximizer::get_center(
    size_t graph_id, NodeId x) {
  vector<NodeId> que = {x};
  unordered_set<NodeId> in_que;
  in_que.insert(x);
  Hash_Edge hash_edge;
  hash_edge.graph_id = graph_id;
  hash_edge.n = G.n;
  hash_edge.forward = true;
  optional<NodeId> center = {};
  if (is_center[x]) center = x;
  for (unsigned int i = 0; i < que.size(); i++) {
    if (center.has_value()) break;
    auto u = que[i];
    for (auto j = G.offset[u]; j < G.offset[u + 1]; j++) {
      auto v = G.E[j];
      auto w = G.W[j];
      if (hash_edge(u, v, w) && in_que.find(v) == in_que.end()) {
        que.push_back(v);
        in_que.insert(v);
        if (is_center[v]) {
          center = v;
          break;
        }
      }
    }
  }
  return {center, que};
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
  sequence<Hash_Edge> hash_edges(R);
  parallel_for(0, R, [&](size_t r) {
    hash_edges[r].graph_id = r;
    hash_edges[r].n = G.n;
    hash_edges[r].forward = true;
    parallel_for(0, center_cnt, [&](size_t i) { labels[r][i] = i; });
  });
  parallel_for(0, G.n, [&](size_t i) {
    NodeId u = i;
    parallel_for(G.offset[i], G.offset[i + 1], [&](size_t j) {
      NodeId v = G.E[j];
      float w = G.W[j];
      parallel_for(0, R, [&](size_t r) {
        if (hash_edges[r](u, v, w)) {
          optional<NodeId> u_center = get_center(r, u).first;
          optional<NodeId> v_center = get_center(r, v).first;
          if (u_center.has_value() && v_center.has_value()) {
            auto u_center_id = center_id[u_center.value()];
            auto v_center_id = center_id[v_center.value()];
            assert(u_center_id < center_cnt);
            assert(v_center_id < center_cnt);
            unite(u_center_id, v_center_id, labels[r]);
          }
        }
      });
    });
  });

  parallel_for(0, R, [&](size_t r) {
    parallel_for(
        0, center_cnt, [&](size_t i) { sketches[r][i] = find(i, labels[r]); },
        1024);
    sequence<NodeId> hist(center_cnt);
    parallel_for(0, center_cnt, [&](size_t i) { hist[i] = 0; });
    for (size_t i = 0; i < center_cnt; i++) {
      NodeId p_label = sketches[r][i];
      hist[p_label]++;
    }
    parallel_for(0, hist.size(), [&](size_t i) {
      if (hist[i] != 0) {
        sketches[r][i] = TOP_BIT | (size_t)hist[i];
      }
    });
  });

  cout << "init_sketches time: " << tt.stop() << endl;
}

sequence<pair<NodeId, float>> CompactInfluenceMaximizer::select_seeds(int k) {
  timer tt;
  sequence<pair<NodeId, float>> seeds(k);
  sequence<size_t> influence(n, n + 1);
  for (int round = 0; round < k; round++) {
    size_t max_influence = 0;
    parallel_for(0, n, [&](size_t i) {
      size_t new_influence = 0;
      if (influence[i] >= max_influence) {
        for (size_t r = 0; r < R; r++) {
          auto [center, que] = get_center(r, i);
          if (center.has_value()) {
            auto c = center_id[center.value()];
            auto p = sketches[r][c];
            if (!(p & TOP_BIT)) {
              p = sketches[r][p];
            }
            new_influence += p & VAL_MASK;
          } else {
            if (!visit[r].test(i)) {
              new_influence += que.size();
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
    parallel_for(0, R, [&](size_t r) {
      auto [center, que] = get_center(r, seed);
      if (center.has_value()) {
        auto c = center_id[center.value()];
        auto p = sketches[r][c];
        if (!(p & TOP_BIT)) {
          sketches[r][p] = TOP_BIT | 0;
        } else {
          sketches[r][c] = TOP_BIT | 0;
        }
      } else {
        for (auto u : que) {
          visit[r].set(u);
        }
      }
    });
  }
  cout << "select_seeds time: " << tt.stop() << endl;
  return seeds;
}
