
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
  DirectedInfluenceMaximizer(const Graph& graph, size_t _R)
      : R(_R), G(graph) {
    n = G.n;
    m = G.m;
  }
  void init_sketches(){
    sketches = sequence<PrunedEstimater>(R);
    parallel_for(0, R, [&](size_t r) {
      sketches[r].init_simple(G, r);
    });
  }
  void init_sketches_phases();
  sequence<pair<NodeId, float>> select_seeds(int k);
  
};

void DirectedInfluenceMaximizer::init_sketches_phases(){
  sketches = sequence<PrunedEstimater>(R);
  cout << "init_sketches_phases " << endl;
  timer t;
  t.start();
  parallel_for(0, R, [&](size_t r){
    sketches[r].scc(G, r);
  });
  cout << "scc: " << t.stop() << endl;
  t.start();
  sequence<sequence<edge>> edge_list(R);
  parallel_for(0, R, [&](size_t r){
    edge_list[r] = sketches[r].pack_edgelist(G);
  });
  cout << "pack edges: " << t.stop() << endl;
  t.start();
  parallel_for(0, R, [&](size_t r){
    size_t n = (sketches[r].DAG).n;
    sketches[r].DAG = EdgeListToGraph(edge_list[r], n);
  });
  cout << "EdgeListToGraph: " << t.stop() << endl;
  t.start();
  parallel_for(0, R, [&](size_t r){
    make_directed(sketches[r].DAG);
  });
  cout << "make directed: " << t.stop() << endl;
  t.start();
  parallel_for(0, R, [&](size_t r){
    sketches[r].compute_weight();
  });
  cout << "compute weight: " << t.stop() << endl;
  t.start();
  parallel_for(0, R, [&](size_t r){
    size_t n = (sketches[r].DAG).n;
    sketches[r].removed = sequence<bool>(n);
    sketches[r].visited = sequence<unsigned int>(n);
    sketches[r].Q = sequence<NodeId>(n);
    sketches[r].time_stamp = 0;
    sketches[r].update = sequence<NodeId>(n);
    parallel_for(0, n, [&](size_t i){
      (sketches[r].removed)[i] = false;
      (sketches[r].visited)[i] = 0;
    });
  });
  cout << "initial sequences: " << t.stop() << endl;
  t.start();
  parallel_for(0, R, [&](size_t r){
    sketches[r].first();
  });
  cout << "first: " << t.stop() << endl;
}
  

sequence<pair<NodeId, float>> DirectedInfluenceMaximizer::select_seeds(int k){
  sequence<pair<NodeId,float>> seeds(k);
  if (k ==0) return seeds;
  sequence<NodeId> sum(n);
  timer add_timer;
  timer update_timer;
  //------------first round-----------
  update_timer.start();
  parallel_for(0, n, [&](size_t i){
    sum[i]=0;
    for(size_t r = 0; r<R; r++){
      if ((UINT_N_MAX - sum[i]) < sketches[r].get_sigma(i)){
        cout << "sum overflow!!!!!!!" << endl;
      }
      sum[i]+= sketches[r].get_sigma(i);
    }
  });
  update_timer.stop();
  auto max_influence_it = parlay::max_element(sum);
  NodeId seed = max_influence_it - sum.begin();
  float influence_gain = sum[seed]/(R+0.0);
  seeds[0]=make_pair(seed, influence_gain);
  add_timer.start();
  parallel_for(0, R, [&](size_t r){
    sketches[r].add(seed);
  });
  add_timer.stop();
  //-----------end of first round ---------
  for (int t = 1; t<k; t++){
    update_timer.start();
    parallel_for(0, R, [&](size_t r){
      sketches[r].update_sum(sum);
    });
    update_timer.stop();
    max_influence_it = parlay::max_element(sum);
    seed = max_influence_it - sum.begin();
    influence_gain = sum[seed]/(R+0.0);
    seeds[t]=make_pair(seed, influence_gain);
    add_timer.start();
    parallel_for(0, R, [&](size_t r){
      sketches[r].add(seed);
    });
    add_timer.stop();
  }
  #if defined (BREAKDOWN)
  cout << "update time: " << update_timer.get_total() << endl;
  cout << "add time: " << add_timer.get_total() << endl;
  #endif
  return seeds;
}
