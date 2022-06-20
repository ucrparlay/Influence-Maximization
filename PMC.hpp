#pragma once

#include <optional>
#include <unordered_set>

#include "get_time.hpp"
#include "graph.hpp"
#include "parlay/sequence.h"
#include "scc.hpp"
using namespace std;
using namespace parlay;

class DirectedInfluenceMaximizer {
 private:
  size_t n, m, R;
  const Graph& G;
  sequence<Hash_Edge> hash_edges;
  sequence<sequence<size_t>> sketches;


 public:
  DirectedInfluenceMaximizer() = delete;
  DirectedInfluenceMaximizer(const Graph& graph, size_t R)
      : R(R), G(graph) {
    n = G.n;
    m = G.m;
    hash_edges = sequence<Hash_Edge>(R);
    parallel_for(0, R, [&](size_t r) {
      hash_edges[r].graph_id = r;
      hash_edges[r].n = G.n;
      hash_edges[r].forward = true;
    });
  }
  void init_sketches();
  sequence<pair<NodeId, float>> select_seeds(int k);
};

void DirectedInfluenceMaximizer::init_sketches(){
    double beta = 1.5;
    bool local_reach = true;
    bool local_scc = true;
    sketches = sequence<sequence<size_t>>(R, sequence<size_t>(n));
    SCC SCC_P(G);
    SCC_P.front_thresh = 1000000000;
    for (size_t r = 0; r < R; r++){
        SCC_P.scc(sketches[r], hash_edges[r], beta, local_reach, local_scc);
    }
    // t_sketch.start();
    // parallel_for(0, R, [&](size_t r) {
    //     SCC SCC_solver(graph);
    //     SCC_solver.front_thresh(SCC_P.front_thresh = 1000000000);
    //     SCC_solver.scc(sketches[r],hash_edge[r], beta, local_reach, local_scc);
    // });
    // cout << "parallel{parallel} cost " << t_sketch.stop() << endl; 
}