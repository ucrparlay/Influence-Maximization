#ifndef BFS_H
#define BFS_H

#include <parlay/parallel.h>
#include <parlay/random.h>

#include <algorithm>
#include <iostream>
#include <queue>  // std::queue

#include "get_time.hpp"
#include "graph.hpp"
#include "utilities.h"

using namespace std;

class BFS {
 private:
  Graph graph;
  sequence<NodeId> front;
  sequence<EdgeId> degree;
  sequence<bool> edge_flag;
  sequence<NodeId> edge_data;
  float w;
  
  size_t n_front;
  size_t n;
  size_t sparse_update(sequence<bool>& dst);

 public:
  size_t rand_seed;
  size_t num_round;
  size_t bfs(const sequence<NodeId>& seeds);
  BFS() = delete;
  BFS(Graph& G, float _w): graph(G), w(_w) {
    n = graph.n;
    front = sequence<NodeId>(n);
    degree = sequence<EdgeId>(n + 1);
    edge_flag = sequence<bool>(graph.m);
    edge_data = sequence<NodeId>(graph.m);
    rand_seed = 0;
  }
};

size_t BFS::sparse_update(sequence<bool>& dst) {
  parallel_for(0,n_front,[&](size_t i){
    NodeId v = front[i];
    degree[i]=graph.offset[v+1]-graph.offset[v];
  });
  auto non_zeros = parlay::scan_inplace(degree.cut(0, n_front));
  degree[n_front] = non_zeros;
  parallel_for(0, n_front,[&](size_t i) {
        NodeId node = front[i];
        parallel_for(degree[i], degree[i + 1],[&](size_t j) {
              size_t self_rand_seed = rand_seed+j;
              EdgeId offset = j - degree[i];
              NodeId ngb_node = graph.E[graph.offset[node] + offset];
              if (_hash((EdgeId)(self_rand_seed + j)) < w*UINT_E_MAX  &&  dst[ngb_node] == false) {
                edge_flag[j] = atomic_compare_and_swap(
                    &dst[ngb_node], false, true);
                if (edge_flag[j]) edge_data[j] = ngb_node;
              } else {
                edge_flag[j] = false;
              }
            },
            2048);
      },
      1);  // may need to see the block size
  rand_seed += non_zeros;
#if defined(DEBUG)
  timer pack_timer;
  pack_timer.start();
#endif
  size_t n_front = parlay::pack_into_uninitialized(edge_data.cut(0, non_zeros),
                                     edge_flag.cut(0, non_zeros), front);
#if defined(DEBUG)
  cout << "sparse pack time " << pack_timer.stop() << endl;
#endif
  return n_front;
}



size_t BFS::bfs(const sequence<NodeId>& seeds) {
  sequence<bool> dst(graph.n);
  parallel_for(0, graph.n, [&](NodeId i) { dst[i] = false; });
  parallel_for(0, seeds.size(), [&](size_t i){
    front[i]=seeds[i];
    dst[i]=true;
  });
  size_t n_visit=0;
  n_front = seeds.size();
  num_round = 0;
  while (n_front > 0) {
    n_visit += n_front;
    num_round++;
    n_front = sparse_update(dst);
  }
  return n_visit;
}

#endif
