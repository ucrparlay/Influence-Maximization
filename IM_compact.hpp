#pragma once

#include <optional>
#include <unordered_set>
#include <queue>
#include <vector>
#include "parlay/random.h"

#include "get_time.hpp"
#include "graph.hpp"
#include "parlay/sequence.h"
#include "union_find.hpp"
#include "pam/pam.h"

using namespace std;
using namespace parlay;

// tree stores <influence, value's index(or saying the vertex id)>
timer t_first;
// NodeId construct_ctl;
// NodeId extract_ctl;
// NodeId update_ctl;
#if defined(EVAL)
size_t num_evals;
#endif
int thresh;

template<typename R, typename Key>
void my_sort(R&& in, R&& temp, Key&& key){
  static_assert(is_random_access_range_v<R>);
  static_assert(std::is_swappable_v<range_reference_type_t<R>>);
  if (in.size() > 1e5){
    // auto key = [&](const pair<NodeId,NodeId>& a){return a.first;};
    // using Key = typename key;
    parlay::internal::integer_sort_<std::true_type, uninitialized_relocate_tag>(make_slice(in), make_slice(temp), make_slice(in), std::forward<Key>(key), (size_t)0, (size_t)0);
  }else{
    auto comp = [&](const pair<NodeId,NodeId>& a, const pair<NodeId, NodeId>& b){return a.first < b.first;};
    sort_inplace(in,comp);
  }
}

class CompactInfluenceMaximizer {
 private:
  size_t n, R, center_cnt, max_influence, queue_size;
  Graph& G;
  float compact_rate;
  sequence<Hash_Edge> hash_edges;
  sequence<bool> is_center, is_seed;
  sequence<size_t> center_id;
  sequence<sequence<NodeId>> sketches;
  // sequence<NodeId> permute;
  sequence<size_t> influence;
  tuple<optional<NodeId>, bool, size_t> get_center(size_t graph_id, NodeId x, NodeId * Q);
  size_t compute(NodeId i);
  size_t compute_pal(NodeId i);
  void construct(sequence<NodeId>& heap, NodeId start, NodeId end, NodeId& root, NodeId gran_ctl);
  NodeId select_winning_tree(sequence<bool>& renew,sequence<NodeId>& heap, int round);
  NodeId select_write_max(int round);
  NodeId select_greedy(int round);
  void update(sequence<NodeId>& heap, sequence<bool>& renew, NodeId start, 
            NodeId end, NodeId& root, NodeId gran_ctl);
  void extract(sequence<NodeId>& heap, sequence<bool>& renew, NodeId start, 
            NodeId end, NodeId gran_ctl);
 public:
  CompactInfluenceMaximizer() = delete;
  CompactInfluenceMaximizer(Graph& graph, float compact_rate, size_t R)
      : R(R), G(graph), compact_rate(compact_rate) {
    assert(G.symmetric);
    n = G.n;
    // search path length within (1/compact_rate)*log n w.h.p
    // where n should be the largest CC size, but here for simplicity, use |V|
    // if the memory usage is dramatically increase, set it to a tighter bound
    queue_size = 2*ceil(1.0/compact_rate)*parlay::log2_up((size_t)n);
    // printf("queue size is %ld\n", queue_size);
    hash_edges = sequence<Hash_Edge>(R);
    parallel_for(0, R, [&](size_t r) {
      hash_edges[r].hash_graph_id = _hash((EdgeId)r);
    });
    is_center = sequence<bool>(n);
    is_seed = sequence<bool>(n);
    center_id = sequence<size_t>(n);
    influence = sequence<size_t>(n);
    #if defined(MEM)
    // cout << "**size of is_center is " << sizeof(bool)*n << endl;
    // cout << "**size of is_seed is " << sizeof(bool)*n << endl;
    // cout << "**size of center_id is " << sizeof(size_t)*n << endl;
    // cout << "**size of influence is "<< sizeof(size_t)*n << endl;
    // cout << "**size of hash_edges is " << sizeof(hash_edges[0])*R << endl;
    #endif
  }
  void init_sketches();
  sequence<pair<NodeId, float>> select_seeds(int k);
  sequence<pair<NodeId, float>> select_seeds_prioQ(int k);
  sequence<pair<NodeId, float>> select_seeds_PAM(int k);
};

// space: O(n / center_cnt)
tuple<optional<NodeId>, bool, size_t> CompactInfluenceMaximizer::get_center(
    size_t graph_id, NodeId x, NodeId* Q) {
  optional<NodeId> center = {};
  if (is_center[x]) {
    return {x, false, 1};
  }
  bool meet_seed = is_seed[x];
  // vector<NodeId> que = {x};
  // unordered_set<NodeId> visit;
  size_t head = 0, tail=0;
  auto visit = [&](NodeId u){
    for (size_t i = 0; i< head; i++){
      if (Q[i]==u){
        return true;
      }
    }
    return false;
  };
  // visit.insert(x);
  Q[head++]=x;
  const auto& hash_edge = hash_edges[graph_id];
  for (size_t i = tail; i < head; i++) {
    if (center.has_value()) break;
    auto u = Q[i];
    for (auto j = G.offset[u]; j < G.offset[u + 1]; j++) {
      auto v = G.E[j];
      if ((u < v ? hash_edge(u, v, G.W[j]) : hash_edge(v, u, G.W[j])) &&
          !visit(v)) {
        Q[head++]=v;
        if (head > queue_size){
          printf("Error, local queue full, queue size is %ld\n", queue_size);
          break;
        }
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
  return {center, meet_seed, head};
}

void CompactInfluenceMaximizer::init_sketches() {
  timer tt;
  tt.start();
  parallel_for(0, n, [&](size_t i) {
    if (_hash((NodeId)i) < compact_rate * UINT_N_MAX){
      is_center[i] = true;
      center_id[i] = 1;
    }
  });
  // cout << "fill is_center, center_id time " << tt.stop() << endl;
  tt.start();
  center_cnt = parlay::scan_inplace(center_id);
  // cout << "scan_inplace time "<< tt.stop() << endl;
  tt.start();

  sketches = sequence(R, sequence<NodeId>(center_cnt));
  // cout << "allocate sequence of sequence time " << tt.stop() << endl;
  tt.start();
  #if defined(MEM)
  // cout << "**size of sketches is " << (sizeof(NodeId)*center_cnt)*R << endl;
  #endif
  auto find = gbbs::find_variants::find_compress;
  auto splice = gbbs::splice_variants::split_atomic_one;
  auto unite =
      gbbs::unite_variants::UniteRemCAS<decltype(splice), decltype(find),
                                        find_atomic_halve>(find, splice);
  sequence<NodeId> label(n);
  sequence<pair<NodeId, NodeId>> belong(n);
  // cout << "allocate label, belong time " << tt.stop() << endl;
  tt.start();
  #if defined(MEM)
  // cout << "--size of label is " << sizeof(NodeId)*n << endl;
  // cout << "--size of belong is " << sizeof(pair<NodeId,NodeId>)*n << endl;
  #endif
  timer t_union_find;
  timer t_sketch;
  timer t_sort;
  timer t_write_sketch;
  // bool v3_v31 = true;
  #if defined(MEM)
  // cout << "--size of offset is " << sizeof(NodeId)*n << endl;
  #endif
  uint max_n_cc=0;
  #if defined(BREAKDOWN)
  cout << "allocating sketching memory time: " << tt.stop() << endl;
  tt.start();
  #endif
  timer scan_timer;
  timer cc_timer;
  sequence<NodeId> center_root(n);
  sequence<NodeId> cc_offset(n+1);
  sequence<NodeId> offset(n);

  sequence<pair<NodeId, NodeId>> sort_helper(n);
  
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
      }, BLOCK_SIZE);
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
    // when n is small, inter_sort is not as good as sort
    auto key = [&](const pair<NodeId,NodeId>& a){return a.first;};
    my_sort(belong, sort_helper, key);
    t_sort.stop();
    t_sketch.start();

    offset[0]=0;
    parallel_for(1, n, [&](size_t i){
      if (belong[i-1].first != belong[i].first){
        offset[i]=1;
      }else{
        offset[i]=0;
      }
    });
    scan_timer.start();
    auto n_cc = scan_inclusive_inplace(offset) +1;
    scan_timer.stop();
    if (n_cc > max_n_cc){
      max_n_cc = n_cc;
    }
    cc_timer.start();
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
    cc_timer.stop();
    t_sketch.stop();
    cc_offset[n_cc]= n;
    t_write_sketch.start();
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
    t_write_sketch.stop();
  }
  // cout << "--size of center_root is " << sizeof(NodeId)*max_n_cc << endl;
  // cout << "--size of cc_offset is " << sizeof(NodeId)*(max_n_cc+1) << endl;
  #if defined(BREAKDOWN)
  cout << "union time time: " << t_union_find.get_total() << endl;
  cout << "sort time: " << t_sort.get_total() << endl;
  cout << "compute sketch time: " << t_sketch.get_total() << endl;
  cout << "   scan time: " << scan_timer.get_total() << endl;
  cout << "   compute time: " << cc_timer.get_total() << endl;
  cout << "write sketch time: " << t_write_sketch.get_total() << endl;
  #endif
  // cout << "init_sketches time: " << tt.get_total() << endl;
}

size_t CompactInfluenceMaximizer::compute(NodeId i){
  size_t new_influence = 0;
  for (size_t r = 0; r < R; r++) {
    NodeId Q[queue_size];
    auto [center, meet_seed, num] = get_center(r, i, Q);
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
  sequence<size_t> new_influence(R);
  parallel_for (0, R, [&](size_t r){
    NodeId Q[queue_size];
    auto [center, meet_seed, num] = get_center(r, i, Q);
    if (center.has_value()) {
      auto c = center_id[center.value()];
      auto p = sketches[r][c];
      if (!(p & TOP_BIT)) {
        p = sketches[r][p];
      }
      new_influence[r] = p & VAL_MASK;
    } else {
      if (!meet_seed) {
        new_influence[r] = num;
      }
    }
  });
  return parlay::reduce(new_influence);
}

void CompactInfluenceMaximizer::construct(sequence<NodeId>& tree, 
                            NodeId start, NodeId end, NodeId& root, NodeId gran_ctl=BLOCK_SIZE){
  if (start == end - 1){
    root=start;
    return;
  }
  NodeId m = (start+end) >>1;
  NodeId left, right;
  if (end - start >  gran_ctl){
    par_do(
      [&](){construct(tree, start, m, left, gran_ctl);},
      [&](){construct(tree, m, end, right, gran_ctl);});
  }else{
    construct(tree, start, m, left, gran_ctl);
    construct(tree, m, end, right, gran_ctl);
  }
  
  
  tree[m]= influence[left]>= influence[right]? left: right;
  root = tree[m];
  return;
}

void CompactInfluenceMaximizer::update(sequence<NodeId>& tree, 
            sequence<bool>& renew,
            NodeId start, NodeId end, NodeId& root, NodeId gran_ctl=BLOCK_SIZE){
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
    if (end-start > gran_ctl){
      par_do(
        [&](){update(tree, renew, start, m, left, gran_ctl);},
        [&](){update(tree, renew, m, end, right, gran_ctl);});
    }else{
      update(tree, renew, start, m, left, gran_ctl);
      update(tree, renew, m, end, right, gran_ctl);
    }
    
    
    tree[m]= influence[left]>= influence[right]? left: right;
    root = tree[m];
    return;
}


void CompactInfluenceMaximizer::extract(
          sequence<NodeId>& tree, 
          sequence<bool>& renew, NodeId start, NodeId end, NodeId gran_ctl){
  if (start +1 == end){
    if (influence[start] >= max_influence){
      if (!renew[start]){
        // influence[start] = compute(permute[start]);
        influence[start] = compute_pal(start);
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
    influence[_root] = compute_pal(_root);
    renew[_root] = true;
    if (influence[_root] > max_influence){
    write_max(&max_influence, influence[_root], 
      [&](size_t a, size_t b){return a<b;});
    }
  }
  if (end - start > gran_ctl){
    par_do(
        [&](){extract(tree, renew, start, m, gran_ctl);},
        [&](){extract(tree, renew, m, end, gran_ctl);});
  }else{
    extract(tree, renew, start, m, gran_ctl);
    extract(tree, renew, m, end, gran_ctl);
  }
  
  return;
}



NodeId CompactInfluenceMaximizer::select_winning_tree(
  sequence<bool>& renew, sequence<NodeId>& heap, int round){
  NodeId seed;
  if (round == 0){
    parallel_for(0, n, [&](size_t i){
      renew[i]= false;
    });
    // has problem
    // update(heap, renew, (NodeId)0, (NodeId)n, seed);
    construct(heap, (NodeId)0, (NodeId)n, seed);
    // max_influence = *(max_element(influence));
  }else{
    max_influence = 0;
    extract(heap, renew,(NodeId)0, (NodeId)n, BLOCK_SIZE);
    #if defined(EVAL)
      size_t _num_evals = count(renew, true);
      num_evals+= _num_evals;
      cout << "re-evaluate: " << _num_evals << endl;
    #endif
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
        influence[i] = compute_pal(i);
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
      influence[i]=compute_pal(i);
    });
  }
  seed = max_element(influence)-influence.begin();
  return seed;  
}

sequence<pair<NodeId, float>> CompactInfluenceMaximizer::select_seeds(int k) {
  timer tt;
  tt.start();
  sequence<pair<NodeId, float>> seeds(k);
  sequence<NodeId> heap(n);
  sequence<bool> renew(n);
  tt.stop();
  // for priority queue
  // auto cmp = [&](NodeId left, NodeId right) {
  //   return (influence[left] <influence[right]); };
	// std::priority_queue<NodeId, vector<NodeId>, decltype(cmp)> Q(cmp);
  
  #if defined(MEM)
  // cout << "**size of seeds is " << sizeof(pair<NodeId,float>)*k << endl;
  // cout << "..size of heap is " << sizeof(NodeId)*n << endl;
  // cout << "..size of renew is " << sizeof(bool)*n << endl;
  #endif
  #if defined(EVAL)
  num_evals=0;
  #endif
  // NodeId seed_idx;
  NodeId seed;
  // first round
  timer t_compute;
  for (int round = 0; round < k; round++) {
    // ---begin winning tree---
    
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
    float influence_gain = influence[seed] / (R + 0.0);
    seeds[round] = {seed, influence_gain};
    // if (round == 0){
    //   cout << "Till first round time: " << t_first.stop() << endl;
    // }
    influence[seed] = 0;
    is_seed[seed] = true;
    renew[seed] = true;
    parallel_for(0, R, [&](size_t r) {
      NodeId Q[queue_size];
      auto center = std::get<0>(get_center(r, seed, Q));
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
    t_compute.stop();
    #if defined(SCORE)
      cout << "scores: "<< seeds[round].second << endl;
    #endif
  }
  #if defined(BREAKDOWN)
  cout << "computing selection time: " << t_compute.get_total() << endl;
  #endif
  #if defined(EVAL)
    cout << "total re-evaluate times: " << num_evals << endl;
  #endif
  return seeds;
}


sequence<pair<NodeId, float>> CompactInfluenceMaximizer::select_seeds_prioQ(int K) {
  timer tt;
  tt.start();
  using key_val = pair<size_t, NodeId>;
  sequence<pair<NodeId, float>> seeds(K);
  auto cmp = [&](key_val left, key_val right) {
		return (left.first < right.first) || (left.first==right.first && left.second > right.second); };
	std::priority_queue<key_val, vector<key_val>, decltype(cmp)> q(cmp);
  sequence<int> iteration(n);
  for (size_t i = 0; i < n; i++) {
		q.push(make_pair(influence[i], i));
    iteration[i]=0;
	}
  #if defined(BREAKDOWN)
  cout << "construct priority queue time: " << tt.stop() << endl;
  #endif
  #if defined(EVAL)
	size_t total_tries = 0;
  #endif
  size_t tries=0;
  
  timer t_compute;
  t_compute.start();
	for (int k = 0; k < K;) {
		const auto node = q.top();
    NodeId u = node.second;
		q.pop();
		if (iteration[u] == k) {
			float score = influence[u]/(R+0.0);
      seeds[k]=make_pair(u,score);
      influence[u] = 0;
      is_seed[u] = true;
      parallel_for(0, R, [&](size_t r) {
        NodeId Q[queue_size];
        auto center = std::get<0>(get_center(r, u, Q));
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
      #if defined(EVAL)
        cout << "re-evaluate: " << tries << endl;
        total_tries+= tries;
      #endif
      #if defined(SCORE)
        cout << "scores: "<< seeds[k].second << endl;
      #endif
      k++; // commit candidate
      tries =0;
		}else {
			tries++;
			influence[u] = compute_pal(u);
			iteration[u] = k;
			q.push(make_pair(influence[u], u));
		}
	}
  t_compute.stop();
  #if defined(BREAKDOWN)
  cout << "computing selection time: " << t_compute.get_total() << endl;
  #endif
  // cout << "select_seeds time: " << tt.get_total() << endl;
  #if defined(EVAL)
  cout << "total re-evaluate times: " << total_tries << endl;
  #endif
  return seeds;
}


sequence<pair<NodeId, float>> CompactInfluenceMaximizer::select_seeds_PAM(int K){
  timer tt;
  tt.start();
  sequence<pair<NodeId, float>> seeds(K);
  using key_type = pair<size_t, NodeId>;
  using value_type = NodeId;
  using par = pair<key_type, value_type>;
  struct entry {
    using key_t = key_type;
    using val_t = value_type; // not used
    // decreasing order, so comp becomes >
    static inline bool comp(key_t a, key_t b) { return (a.first == b.first)? a.second<b.second: a.first>b.first;}
  }; 
  using tmap = pam_map<entry>;
  auto gen_par = [&](NodeId i) -> par { return make_pair(make_pair(influence[i],i),i);};
  tmap m1;
  tmap::reserve((NodeId)n);
  m1 = tmap::multi_insert(m1, delayed_seq<par>((NodeId)n, gen_par));
  #if defined(DEBUG)
  cout << "generate m1 from influence[.]" << endl;
  #endif
  int n_rounds = parlay::log2_up(n);
  sequence< NodeId> B(n);
  #if defined(EVAL)
  sequence<bool> B_flag(n);
  size_t total_tries = 0;
  #endif
  
  
  tmap::node* res;
  tmap::node* rem;
  rem = m1.root;
  #if defined(BREAKDOWN)
  cout << "construct priority queue time: " << tt.stop() << endl;
  #endif
  // size_t remain_size = n;
  timer t_compute;
  t_compute.start();
  for (int k = 0; k<K; k++){
    #if defined(DEBUG)
    cout << "k = " << k << endl;
    #endif
    size_t offset_ = 0;
    key_type id = std::make_pair<size_t, NodeId>(0, 4294967295);
    key_type seed = id;
    size_t step = 0;
    for (int round = 0; round<n_rounds ; round++){
      key_type key;
      #if defined(DEBUG)
      cout << " round " << round << endl;
      cout << " tree size is " << tmap::Tree::size(rem) << endl;
      #endif
      if ((size_t)1<<round >= tmap::Tree::size(rem)){
        step = tmap::Tree::size(rem)-1;
      }else{
        step = 1<<round;
      }
      #if defined(DEBUG)
        cout << "   step " << step << endl;
      #endif
      auto select_ans = tmap::Tree::select(rem, step); // rank starts from 0
      if (!select_ans){
        cout << "select not exists" << endl;
        break;
      }
      key = tmap::Tree::get_key(select_ans);
      #if defined(DEBUG)
      cout << "     select key is " << key.first << ","<<key.second << endl;
      #endif
      auto bsts = tmap::Tree::split(rem, key);
      #if defined(DEBUG)
      cout << "     split tree" << endl;
      #endif
      res = bsts.first;
      rem = bsts.second;
      // initial max_influence as 0 or seed's, the results are the same.
      // Because all the old influences in this batch is greater than seed's.
      max_influence = 0;
      auto re_compute = [&](tmap::E& e, size_t i) {
        NodeId node = e.second;
        if (influence[node] >= max_influence){
          influence[node]=compute_pal(node);
          #if defined(EVAL)
            B_flag[offset_+i]=true;
          #endif
          if (influence[node]> max_influence){
            write_max(&max_influence, influence[node], 
                  [&](size_t a, size_t b){return a < b;});
          }
        }else{
          #if defined(EVAL)
            B_flag[offset_+i]=false;
          #endif
        }
        B[offset_+i]=node;
      };
      size_t granularity = utils::node_limit;
      // size_t granularity = 10;
      tmap::Tree::foreach_index(res, 0, re_compute, granularity, true);
      #if defined(DEBUG)
      cout << "   finish recompute" << endl;
      #endif
      auto red_f = [&](const key_type& l,
                    const key_type& r) {
          return entry::comp(l,r) ? l : r;
      };
      auto monoid = parlay::make_monoid(red_f, id);
      auto keys_r = parlay::delayed_seq<key_type>(step, [&](size_t i) { // check
      NodeId node = B[offset_+i]; 
      return make_pair(influence[node],node);});
      auto max_res = parlay::reduce(keys_r, monoid);
      #if defined(DEBUG)
      cout << "   reduce max_res " << max_res.first << "," << max_res.second << endl;
      #endif
      if (seed == id || entry::comp(max_res, seed)){
        seed = max_res;
      }
      offset_ += step;
      #if defined(DEBUG)
      cout << "   seed is " << seed.second << endl;
      #endif
      if (tmap::Tree::size(rem) == 0){
        NodeId node = key.second;
        influence[node]=compute_pal(node);
        B[offset_]=node;
        #if defined(EVAL)
          B[offset_]=true;
        #endif
        offset_++;
        key_type new_key = make_pair(influence[node], node);
        if (entry::comp(new_key,seed)){
          seed = new_key;
        }
        break;
      }else{
        auto replace = [] (const tmap::V& a, const tmap::V& b) {return b;}; 
        rem = tmap::Tree::insert(rem, make_pair(key,key.second), replace); // the value of the key doesn't matter
        if (entry::comp(seed, key)){
          break;
        }
      }
    }
    seeds[k]=make_pair(seed.second, seed.first/(R+0.0));
    #if defined(SCORE)
      cout << "scores: "<< seeds[k].second << endl;
    #endif
    influence[seed.second] = 0;
    is_seed[seed.second] = true;
    parallel_for(0, R, [&](size_t r) {
      NodeId Q[queue_size];
      auto center = std::get<0>(get_center(r, seed.second, Q));
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
    #if defined (DEBUG)
    cout << " offet before insert back " << offset_ << endl;
    #endif
    auto make_par = [&](size_t i) -> par { NodeId node = B[i]; return make_pair(make_pair(influence[node],node),node); };
    // m1 = tmap::multi_insert(m1, delayed_seq<par>(offset_, make_par));
    auto replace = [] (const tmap::V& a, const tmap::V& b) {return b;}; 
    // parlay::sequence<tmap::E> A = tmap::Build::sort_remove_duplicates(delayed_seq<par>(offset_, make_par));
    // constexpr static auto less = [] (entry a, entry b) {
    //   return entry::comp(entry::get_key(a), entry::get_key(b));};
    parlay::sequence<tmap::E> A = parlay::sort(delayed_seq<par>(offset_, make_par), [&](const tmap::E a, const tmap::E b){return entry::comp(a.first, b.first);});
    #if defined(DEBUG)
    cout << " multi insert sequence size " << A.size() << endl;
    #endif
    rem = tmap::Tree::multi_insert_sorted(rem, A.data(), A.size(), replace);
    #if defined(EVAL)
    size_t re_evals = count(B_flag.cut(0, offset_), true);
    // cout << "re-evaluate: " << re_evals << endl;
    total_tries+= re_evals;
    #endif
  }
  t_compute.stop();
  #if defined(BREAKDOWN)
  cout << "computing selection time: " << t_compute.get_total() << endl;
  #endif
  #if defined(EVAL)
  cout << "total re-evaluate times: " << total_tries << endl;
  #endif
  return seeds;
}
