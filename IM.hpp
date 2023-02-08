#pragma once


#include "graph.hpp"
#include "union_find_rules.hpp"
#include "get_time.hpp"
#include "pam/pam.h"
using namespace std;
using namespace parlay;





class InfluenceMaximizer{
  private:
    size_t n, m;
    size_t R;
    Graph& G;
    sequence<size_t> sketches;  // R*n
    sequence<size_t> influence; // R
    size_t compute(NodeId u);
    void construct(sequence<NodeId>& tree, NodeId start,
                NodeId end, NodeId& root);
    void update(sequence<NodeId>& tree, sequence<bool>& renew,
                NodeId start, NodeId end, NodeId& root);
    void extract(sequence<NodeId>& tree, 
          sequence<bool>& renew, NodeId start, NodeId end);
    NodeId select_winning_tree(sequence<bool>& renew,
          sequence<NodeId>& heap, int round);
    
    
    
  public:
    InfluenceMaximizer() = delete;
    InfluenceMaximizer(Graph & graph, size_t _R):G(graph), R(_R){
        n = G.n;
        m = G.m;
        sketches = sequence<size_t>(n*R);
        influence = sequence<size_t>(n);
    }
    void init_sketches();
    sequence<pair<NodeId, float>> select_seeds(int k);
    sequence<pair<NodeId, float>> select_seeds_Q(int k);
    sequence<pair<NodeId, float>> select_seeds_PAM(int k);
};




void InfluenceMaximizer::init_sketches(){
  sequence<Hash_Edge> hash_edges(R);
  sequence<size_t> label(n*R);
  auto find = gbbs::find_variants::find_compress;
  auto splice = gbbs::splice_variants::split_atomic_one;
  auto unite =
      gbbs::unite_variants::UniteRemCAS<decltype(splice), decltype(find),
                                        find_atomic_halve>(find, splice);
  parallel_for(0, R, [&](size_t r) {
    hash_edges[r].hash_graph_id = _hash((EdgeId)r);
    parallel_for(0, n, [&](size_t v){
      size_t index = n*r + v;
      label[index]=v;
      sketches[index]=0;
    });
  });
  parallel_for(0, n, [&](size_t u) {
    parallel_for(G.offset[u], G.offset[u + 1], [&](size_t j) {
      auto v = G.E[j];
      if (u<v){
        parallel_for(0, R, [&](size_t r){
          if (hash_edges[r](u,v,G.W[j])){
            //  use part of label, but not allocate new memory
            unite(u,v, label.subseq(r*n, (r+1)*n));
          }
        });
      }
    });
  });
  parallel_for(0, n*R, [&](size_t i){
    size_t u = i % n;
    size_t r = (i-u)/n;
    size_t l = find(u, label.subseq(r*n, (r+1)*n));
    size_t index= u*R+r;
    fetch_and_add(&sketches[index], (size_t)1);
  });
  parallel_for(0, n*R, [&](size_t i){
    size_t u = i % n;
    size_t r = (i-u)/n;
    size_t l = label[i];
    size_t index= u*R+r;
    if (u == l){
      fetch_and_add(&influence[u], sketches[index]);
      sketches[index]= TOP_BIT | sketches[index];
    }else{
      sketches[index]=l;
    }
  });
}


size_t InfluenceMaximizer::compute(NodeId u){
  auto get_infl = [&](size_t r) -> size_t {
    size_t p = sketches[u*R + r];
    if (!(p & TOP_BIT)){
      p = sketches[p*R+r];
    }
    return VAL_MASK & p; 
  };
  return parlay::reduce(delayed_seq<size_t>((size_t)R, get_infl));
}

void InfluenceMaximizer::construct(sequence<NodeId>& tree, 
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

void InfluenceMaximizer::update(sequence<NodeId>& tree, 
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


void InfluenceMaximizer::extract(
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



NodeId InfluenceMaximizer::select_winning_tree(sequence<bool>& renew,
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
    #if defined(EVAL)
      size_t _num_evals = count(renew, true);
      num_evals+= _num_evals;
      // cout << "re-evaluate: " << _num_evals << endl;
    #endif
    update(heap, renew, (NodeId)0, (NodeId)n, seed);
  }
  return seed;
}

sequence<pair<NodeId, float>> InfluenceMaximizer::select_seeds(int k) {
  timer tt;
  sequence<pair<NodeId, float>> seeds(k);
  sequence<NodeId> heap(n);
  sequence<bool> renew(n);
  // for priority queue
  // auto cmp = [&](NodeId left, NodeId right) {
  //   return (influence[left] <influence[right]); };
	// std::priority_queue<NodeId, vector<NodeId>, decltype(cmp)> Q(cmp);
  
  #if defined(MEM)
  cout << "**size of seeds is " << sizeof(pair<NodeId,float>)*k << endl;
  cout << "..size of heap is " << sizeof(NodeId)*n << endl;
  cout << "..size of renew is " << sizeof(bool)*n << endl;
  #endif
  #if defined(EVAL)
  num_evals=0;
  #endif
  // NodeId seed_idx;
  NodeId seed;
  // first round
  for (int round = 0; round < k; round++) {
    tt.start();
    seed = select_winning_tree(renew, heap, round);
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
    #if defined(SCORE)
      cout << "scores: "<< seeds[round].second << endl;
    #endif
  }
  cout << "select_seeds time: " << tt.get_total() << endl;
  #if defined(EVAL)
    cout << "total re-evaluate times: " << num_evals << endl;
  #endif
  return seeds;
}
