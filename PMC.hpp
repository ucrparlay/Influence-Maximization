
#pragma once

#include <optional>
#include <unordered_set>

#include "get_time.hpp"
#include "graph.hpp"
#include "parlay/sequence.h"
// #include "scc.hpp"
#include "PrunedEstimater.hpp"
using namespace std;
using namespace parlay;

class DirectedInfluenceMaximizer {
 private:
  size_t n, m, R;
  const Graph& G;
  sequence<Hash_Edge> hash_edges;
  sequence<PrunedEstimater> sketches;

 public:
  DirectedInfluenceMaximizer() = delete;
  DirectedInfluenceMaximizer(const Graph& graph, size_t R)
      : R(R), G(graph) {
    n = G.n;
    m = G.m;
  }
  void init_sketches(){
    sketches = sequence<PrunedEstimater>(R);
    parallel_for(0, R, [&](size_t r) {
      sketches[r].init_simple(G, r);
    });
    // for (size_t r = 0; r<R; r++){
    //   sketches[r].init_simple(G,r);
    // }
  }
  sequence<pair<NodeId, float>> select_seeds(int k, bool CELF);
};

sequence<pair<NodeId, float>> DirectedInfluenceMaximizer::select_seeds(int k, bool CELF){
  sequence<pair<NodeId,float>> seeds(k);
  if (k ==0) return seeds;
  sequence<NodeId> sum(n);
  //------------first round-----------
  parallel_for(0, n, [&](size_t i){
    sum[i]=0;
    for(size_t r = 0; r<R; r++){
      sum[i]+= sketches[r].sigma(i);
    }
  });
  auto max_influence_it = parlay::max_element(sum);
  NodeId seed = max_influence_it - sum.begin();
  float influence_gain = sum[seed]/(R+0.0);
  seeds[0]=make_pair(seed, influence_gain);
  parallel_for(0, R, [&](size_t r){
    sketches[r].add(seed);
  });
  //-----------end of first round ---------
  for (int t = 1; t<k; t++){
    size_t max_sum = 0;
    parallel_for(0, n, [&](size_t i){
      size_t new_sum = 0;
      if ((!CELF) || sum[i] >= max_sum){
        for (size_t r = 0; r<R; r++){
          new_sum += sketches[r].sigma(i); 
        }
        sum[i] = new_sum;
      }
      if (CELF && (new_sum > max_sum)){
        write_max(&max_sum, new_sum, [](size_t a, size_t b){return a<b;});
      }
    },1024);
    if (CELF){
      max_influence_it = parlay::find(make_slice(sum), max_sum);
    }else{
      max_influence_it = parlay::max_element(sum);
    }
    seed = max_influence_it - sum.begin();
    influence_gain = max_sum/(R+0.0);
    seeds[t]=make_pair(seed, influence_gain);
    parallel_for(0, R, [&](size_t r){
      sketches[r].add(seed);
    });

  }
  return seeds;
}
