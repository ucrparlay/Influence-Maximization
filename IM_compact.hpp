#pragma once

#include <optional>
#include <unordered_set>
#include "parlay/random.h"

#include "get_time.hpp"
#include "graph.hpp"
#include "parlay/sequence.h"
#include "union_find.hpp"
// #include "WinnerTree.hpp"
using namespace std;
using namespace parlay;

// tree stores <influence, value's index(or saying the vertex id)>
timer t_first;

int thresh;
class CompactInfluenceMaximizer {
 private:
  size_t n, R, center_cnt, max_influence;
  Graph& G;
  float compact_rate;
  sequence<Hash_Edge> hash_edges;
  sequence<bool> is_center, is_seed;
  sequence<size_t> center_id;
  sequence<sequence<NodeId>> sketches;
  // sequence<NodeId> permute;
  sequence<size_t> influence;

  tuple<optional<NodeId>, bool, size_t> get_center(size_t graph_id, NodeId x);
  size_t compute(NodeId i);
  void construct(sequence<NodeId>& heap, NodeId start, NodeId end, NodeId& root);
  NodeId select_winning_tree(sequence<bool>& renew,sequence<NodeId>& heap, int round);
  NodeId select_write_max(int round);
  NodeId select_greedy(int round);
  void update(sequence<NodeId>& heap, sequence<bool>& renew, NodeId start, NodeId end, NodeId& root);
  void extract(sequence<NodeId>& heap, sequence<bool>& renew, NodeId start, NodeId end);
 public:
  CompactInfluenceMaximizer() = delete;
  CompactInfluenceMaximizer(Graph& graph, float compact_rate, size_t R)
      : R(R), G(graph), compact_rate(compact_rate) {
    assert(G.symmetric);
    n = G.n;
    hash_edges = sequence<Hash_Edge>(R);
    parallel_for(0, R, [&](size_t r) {
      hash_edges[r].hash_graph_id = _hash((EdgeId)r);
    });
    is_center = sequence<bool>(n);
    is_seed = sequence<bool>(n);
    center_id = sequence<size_t>(n);
    influence = sequence<size_t>(n);
    cout << "**size of is_center is " << sizeof(bool)*n << endl;
    cout << "**size of is_seed is " << sizeof(bool)*n << endl;
    cout << "**size of center_id is " << sizeof(size_t)*n << endl;
    cout << "**size of influence is "<< sizeof(size_t)*n << endl;
    cout << "**size of hash_edges is " << sizeof(hash_edges[0])*R << endl;
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
    if (_hash((NodeId)i) < compact_rate * UINT_N_MAX){
      is_center[i] = true;
      center_id[i] = 1;
    }
  });
  center_cnt = parlay::scan_inplace(center_id);

  sketches = sequence(R, sequence<NodeId>(center_cnt));
  cout << "**size of sketches is " << (sizeof(NodeId)*center_cnt)*R << endl;
  auto find = gbbs::find_variants::find_compress;
  auto splice = gbbs::splice_variants::split_atomic_one;
  auto unite =
      gbbs::unite_variants::UniteRemCAS<decltype(splice), decltype(find),
                                        find_atomic_halve>(find, splice);
  sequence<NodeId> label(n);
  sequence<pair<NodeId, NodeId>> belong(n);
  cout << "--size of label is " << sizeof(NodeId)*n << endl;
  cout << "--size of belong is " << sizeof(pair<NodeId,NodeId>)*n << endl;

  timer t_union_find;
  timer t_sketch;
  timer t_sort;
  // bool v3_v31 = true;
  cout << "--size of offset is " << sizeof(NodeId)*n << endl;
  uint max_n_cc=0;
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
    // if (find(3, label) != find(31, label)){
    //   v3_v31 = false;
    // }
    t_union_find.stop();
    t_sort.start();
    // sort_inplace(belong);
    integer_sort_inplace(belong, [&](pair<NodeId,NodeId> a){return a.first;});
    t_sort.stop();
    t_sketch.start();

    sequence<NodeId> offset(n);
    offset[0]=0;
    parallel_for(1, n, [&](size_t i){
      if (belong[i-1].first != belong[i].first){
        offset[i]=1;
      }else{
        offset[i]=0;
      }
    });
    auto n_cc = scan_inclusive_inplace(offset) +1;
    if (n_cc > max_n_cc){
      max_n_cc = n_cc;
    }
    sequence<NodeId> center_root(n_cc);
    sequence<NodeId> cc_offset(n_cc+1);
    parallel_for(0,n,[&](size_t i){
      if ( i== 0 || belong[i-1].first != belong[i].first){
        NodeId cc_i = offset[i];
        assert(cc_i < n_cc);
        cc_offset[cc_i] = i;
        for (auto j = i; j<n; j++){
          if (j > i && belong[j].first!=belong[j-1].first){
            break;
          }
          NodeId v = belong[j].second;
          if (is_center[v]){
            center_root[cc_i] = center_id[v];
            break;
          }
        }
      }
    });
    cc_offset[n_cc]= n;
    parallel_for(0, n, [&](size_t i){
      NodeId i_idx = offset[i];
      assert(i_idx < n_cc);
      NodeId cc_cnt = cc_offset[i_idx+1] - cc_offset[i_idx];
      NodeId root = center_root[i_idx];
      NodeId v = belong[i].second;
      // _influence[v]+= cc_cnt;
      influence[v]+=cc_cnt;
      if (is_center[v]){
        NodeId center_i = center_id[v];
        if (center_i == root){
          sketches[r][center_i] = TOP_BIT | cc_cnt;
        }else{
          sketches[r][center_i] = root;
        }
      }
    });
    t_sketch.stop();
  }
  // parallel_for(0, n, [&](size_t i){
  //   NodeId v = permute[i];
  //   influence[i]=_influence[v];
  // });
  cout << "--size of center_root is " << sizeof(NodeId)*max_n_cc << endl;
  cout << "--size of cc_offset is " << sizeof(NodeId)*(max_n_cc+1) << endl;
  cout << "init_sketches time: " << tt.stop() << endl;
  cout << "union time time: " << t_union_find.get_total() << endl;
  cout << "sort time: " << t_sort.get_total() << endl;
  cout << "sketch time: " << t_sketch.get_total() << endl;
  // cout << "vertex 3 is equal to vertex 31: " << v3_v31 << endl;
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

void CompactInfluenceMaximizer::construct(sequence<NodeId>& tree, 
                            NodeId start, NodeId end, NodeId& root){
  if (start == end - 1){
    root=start;
    return;
  }
  NodeId m = (start+end) >>1;
  NodeId left, right;
  par_do(
      [&](){construct(tree, start, m, left);},
      [&](){construct(tree, m, end, right);});
  
  tree[m]= influence[left]>= influence[right]? left: right;
  root = tree[m];
  return;
}

void CompactInfluenceMaximizer::update(sequence<NodeId>& tree, 
            sequence<bool>& renew,
            NodeId start, NodeId end, NodeId& root){
    if (start == end - 1){
        // need to check tht logic
        if (renew[start]){
          renew[start]=false;
        }
        root=start;
        return;
    }
    
    NodeId m = (start+end) >>1;
    NodeId _root = tree[m];
    if (!renew[_root]){
      root = _root;
      return;
    }
    NodeId left, right;
    par_do(
        [&](){update(tree, renew, start, m, left);},
        [&](){update(tree, renew, m, end, right);});
    
    tree[m]= influence[left]>= influence[right]? left: right;
    root = tree[m];
    return;
}


void CompactInfluenceMaximizer::extract(
          sequence<NodeId>& tree, 
          sequence<bool>& renew, NodeId start, NodeId end){
  if (start +1 == end){
    if (influence[start] >= max_influence){
      if (!renew[start]){
        // influence[start] = compute(permute[start]);
        influence[start] = compute(start);
        renew[start] = true;
        if (influence[start] > max_influence){
          write_max(&max_influence, influence[start], 
          [&](size_t a, size_t b){return a < b;});
        }
      }
    }
    return;
  }
  NodeId m = (start+end)>>1;
  auto _root = tree[m];
  // printf("root %u start %u end %u influence %ld \n", _root, start,end,influence[_root]);
  if (influence[_root] < max_influence && !renew[_root]){
    return;
  }
  if (influence[_root] >= max_influence){
    // in this case, renew[root] must be false
    // influence[_root] = compute(permute[_root]);
    influence[_root] = compute(_root);
    renew[_root] = true;
    if (influence[_root] > max_influence){
    write_max(&max_influence, influence[_root], 
      [&](size_t a, size_t b){return a<b;});
    }
  }
  par_do(
        [&](){extract(tree, renew, start, m);},
        [&](){extract(tree, renew, m, end);});
  return;
}

NodeId CompactInfluenceMaximizer::select_winning_tree(sequence<bool>& renew,
                                                      sequence<NodeId>& heap, int round){
  NodeId seed;
  if (round == 0){
    parallel_for(0, n, [&](size_t i){
      renew[i]= false;
    });
    // has problem
    // update(heap, renew, (NodeId)0, (NodeId)n, seed);
    construct(heap, (NodeId)0, (NodeId)n, seed);
    max_influence = *(max_element(influence));
  }else{
    max_influence = 0;
    extract(heap, renew,(NodeId)0, (NodeId)n);
    update(heap, renew, (NodeId)0, (NodeId)n, seed);
  }
  return seed;
}

NodeId CompactInfluenceMaximizer::select_write_max(int round){
  if (round > 0){
    max_influence = 0;
    parallel_for(0,n,[&](size_t i){
      if (influence[i] >= max_influence){
        // influence[i]=compute(permute[i]);
        influence[i] = compute(i);
        if (influence[i]>max_influence){
          write_max(&max_influence, influence[i], 
          [&](size_t a, size_t b){return a < b;});
        }
      }
    });
  }
  
  NodeId seed = parlay::max_element(influence) - influence.begin();
  return seed;
}

NodeId CompactInfluenceMaximizer::select_greedy(int round){
  NodeId seed; 
  if (round >0){
    parallel_for(0, n, [&](size_t i){
      // influence[i]=compute(permute[i]);
      influence[i]=compute(i);
    });
  }
  seed = max_element(influence)-influence.begin();
  return seed;  
}

sequence<pair<NodeId, float>> CompactInfluenceMaximizer::select_seeds(int k) {
  timer tt;
  sequence<pair<NodeId, float>> seeds(k);
  sequence<NodeId> heap(n);
  sequence<bool> renew(n);
  cout << "**size of seeds is " << sizeof(pair<NodeId,float>)*k << endl;
  cout << "..size of heap is " << sizeof(NodeId)*n << endl;
  cout << "..size of renew is " << sizeof(bool)*n << endl;
  // NodeId seed_idx;
  NodeId seed;
  // first round
  for (int round = 0; round < k; round++) {
    tt.start();
    // ---beging winning tree---
    // seed_idx = select_winning_tree(renew, heap, round);
    seed = select_winning_tree(renew, heap, round);
    
    // ---end winning tree---

    // ---begin write max---
    // seed = select_write_max(round);
    // ---end write max---

    // ---begin greedy ----
    // seed = select_greedy(round);
    // if (round == 1){
    //   cout << "round 1: " ;
    //   parallel_for(0, n, [&](size_t i){
    //     if (influence[i]==0){
    //       printf("%ld ", i);
    //     }
    //   });
    //   cout << endl;
    // }
    // --- end greedy ---
    // auto influence_max = max_element(influence);
    // auto id = influence_max-influence.begin();
    // cout << "round " << round <<  " seed " << seed << " influence " << influence[seed] << endl;
    // cout << "max_influence " << max_influence << endl;
    // cout << seed << " " << influence[seed] << endl;
    // assert((id == seed) && "selected seed match with max(influence)");
    // cout << "max(influence) " << *influence_max << " id " << id << " compute[id] " << compute(id) <<endl;
    float influence_gain = influence[seed] / (R + 0.0);
    seeds[round] = {seed, influence_gain};
    if (round == 0){
      cout << "Till first round time: " << t_first.stop() << endl;
    }
    influence[seed] = 0;
    is_seed[seed] = true;
    renew[seed] = true;
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
