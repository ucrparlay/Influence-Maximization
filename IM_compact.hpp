#pragma once

#include <optional>
#include <unordered_set>

#include "get_time.hpp"
#include "graph.hpp"
#include "parlay/sequence.h"
#include "union_find.hpp"
#include "WinnerTree.hpp"
using namespace std;
using namespace parlay;

int thresh;
class CompactInfluenceMaximizer {
 private:
  size_t n, m, R, center_cnt;
  Graph& G;
  float compact_rate;
  sequence<Hash_Edge> hash_edges;
  sequence<bool> is_center, is_seed;
  sequence<size_t> center_id;
  sequence<sequence<size_t>> sketches;
  sequence<size_t> influence;

  tuple<optional<NodeId>, bool, size_t> get_center(size_t graph_id, NodeId x);
  size_t compute(NodeId i);
  size_t compute_pal(NodeId i);
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
    influence = sequence<size_t>(n);
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
  for (size_t i = 0; i < que.size(); i++) {
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

  parallel_for(0, n, [&](size_t i) {
    Hash_Edge hash_edge;  // use hash_edge as random generator
    hash_edge.graph_id = i + R;
    hash_edge.n = G.n;
    hash_edge.forward = true;
    if (hash_edge(i, i, compact_rate)) {
      is_center[i] = true;
      center_id[i] = 1;
    }
    influence[i]=0;
  });
  center_cnt = parlay::scan_inplace(center_id);
  cout << "center_cnt: " << center_cnt << endl;

  sketches = sequence(R, sequence<size_t>(center_cnt));
  auto find = gbbs::find_variants::find_compress;
  auto splice = gbbs::splice_variants::split_atomic_one;
  auto unite =
      gbbs::unite_variants::UniteRemCAS<decltype(splice), decltype(find),
                                        find_atomic_halve>(find, splice);
  sequence<NodeId> label(n);
  sequence<pair<NodeId, NodeId>> belong(n);
  timer t_union_find;
  timer t_sketch;
  timer t_sort;
  for (size_t r = 0; r < R; r++) {
    // cout << "r = " << r << endl;
    t_union_find.start();
    parallel_for(0, n, [&](size_t i){
      label[i]=i;
    });
    parallel_for(0, n, [&](size_t u) {
      parallel_for(G.offset[u], G.offset[u + 1], [&](size_t j) {
        auto v = G.E[j];
        if (u < v && hash_edges[r](u, v, G.W[j])) {
          unite(u,v,label);
        }
      });
    });
    parallel_for(0, n, [&](size_t i){
      auto l =find(i,label);
      belong[i]=make_pair(l,i);
    });
    t_union_find.stop();
    t_sort.start();
    // sort_inplace(belong);
    integer_sort_inplace(belong, [&](pair<NodeId,NodeId> a){return a.first;});
    // auto group = group_by_key(belong);
    t_sort.stop();
    t_sketch.start();
    // parallel_for(0, group.size(), [&](size_t i){
    //   auto key = group[i];
    //   NodeId center;
    //   bool meet_center = false;
    //   NodeId cc_cnt=0;
    //   for (auto j : key.second){
    //     if (is_center[j]){
    //       if (!meet_center){
    //         meet_center = true;
    //         center = center_id[j];
    //       }else{
    //         sketches[r][center_id[j]] = center;
    //       }
    //     }
    //     cc_cnt++;
    //   }
    //   if (meet_center){
    //     sketches[r][center] = TOP_BIT | cc_cnt;
    //   }
    // });
    parallel_for(0, n, [&](size_t i){
      if (i == 0 || belong[i].first != belong[i-1].first){
        NodeId center;
        bool meet_center = false;
        NodeId cc_cnt=0;
        for (auto j = i; j<n; j++){
          if (j>i && belong[j].first!= belong[j-1].first){
            break;
          }
          if (is_center[j]){
            if (!meet_center){
              meet_center = true;
              center = center_id[j];
            }else{
              sketches[r][center_id[j]] = center;
            }
          }
          cc_cnt++;
        }
        if (meet_center){
          sketches[r][center] = TOP_BIT | cc_cnt;
        }
      }
    });
    t_sketch.stop();
  }
  cout << "init_sketches time: " << tt.stop() << endl;
  cout << "union time time: " << t_union_find.get_total() << endl;
  cout << "sort time: " << t_sort.get_total() << endl;
  cout << "sketch time: " << t_sketch.get_total() << endl;
}

size_t CompactInfluenceMaximizer::compute(NodeId i){
  size_t new_influence = 0;
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
  return new_influence;
}

size_t CompactInfluenceMaximizer::compute_pal(NodeId i){
  sequence<size_t> new_influences(R);
  parallel_for(0, R, [&](int r){
    auto [center, meet_seed, num] = get_center(r, i);
    if (center.has_value()) {
      auto c = center_id[center.value()];
      auto p = sketches[r][c];
      if (!(p & TOP_BIT)) {
        p = sketches[r][p];
      }
      new_influences[r] = p & VAL_MASK;
    } else {
      if (!meet_seed) {
        new_influences[r] = num;
      }
    }
  });
  return reduce(new_influences);
}

sequence<pair<NodeId, float>> CompactInfluenceMaximizer::select_seeds(int k) {
  cout << "threshold " << thresh << endl;
  timer tt;
  sequence<pair<NodeId, float>> seeds(k);
  sequence<int> time_stamp(n); // 
  sequence<pair<size_t, NodeId>> heap(n);
  NodeId seed;
  // first round
  for (int round = 0; round < k; round++) {
    if (round == 0){
      tt.start();
      parallel_for(0, n, [&](size_t i){
        influence[i]= compute(i);
        time_stamp[i]= round;
      });
      // build_up(influence, heap, (NodeId)0, (NodeId)n);
      seed = parlay::max_element(influence) - influence.begin();
      cout << seed <<" " << 0 << " " <<tt.stop() << endl;
    }else if (round < thresh){
      tt.start();
      size_t max_influence = 0;
      parallel_for(0,n,[&](size_t i){
        if (influence[i]>max_influence){
          auto new_influence = compute(i);
          if (new_influence > max_influence){
            write_max(&max_influence, new_influence, 
              [](size_t a, size_t b){return a<b;});
          }
          influence[i]=new_influence;
          time_stamp[i]=round;
        }
      });
      seed = parlay::max_element(influence) - influence.begin();
      cout << seed << " write_max " << tt.stop() << endl;
    }else{
      tt.start();
      if (round == thresh){
        build_up(influence, heap, (NodeId)0, (NodeId)n);
      }
      NodeId eval_cnt=0;
      NodeId m = n>>1;
      NodeId root_node = heap[m].second;
      while (time_stamp[root_node] != round){
        size_t new_influence = compute_pal(root_node);
        influence[root_node] = new_influence;
        time_stamp[root_node]=round;
        update(influence, heap, (NodeId)0, (NodeId)n);
        root_node = heap[m].second;
        eval_cnt++;
      }
      seed = root_node;
      cout << seed << " " << eval_cnt << " " << tt.stop() << endl;
    }
    tt.start();
    float influence_gain = influence[seed] / (R + 0.0);
    seeds[round] = {seed, influence_gain};
    influence[seed] = 0;
    update(influence, heap, (NodeId)0, (NodeId)n);
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
    tt.stop();
  }
  cout << "select_seeds time: " << tt.get_total() << endl;
  return seeds;
}
