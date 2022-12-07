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

#define hash_edge_id(j,w) (_hash((EdgeId)(rand_seed + j)) < w*UINT_E_MAX)
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
  Hash_Edge hash_edge;
  size_t rand_seed;
  size_t num_round;
  size_t bfs(const sequence<NodeId>& seeds, Hash_Edge _hash_edge);
  size_t bfs_sequence(const sequence<NodeId>& seeds, Hash_Edge _hash_edge);
  size_t get_n(){return n;}
  size_t get_m(){return graph.m;}
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
        NodeId u = front[i];
        parallel_for(0, graph.offset[u+1]-graph.offset[u],[&](size_t j) {
          EdgeId offset = degree[i];
          NodeId v = graph.E[graph.offset[u] + j];
          float _w = graph.W[graph.offset[u]+j];
          if (hash_edge_id(graph.offset[u]+j, _w) && dst[v]==false){
          // if ((dst[v] == false) && hash_edge(u, v, w)){
            edge_flag[offset+j] = atomic_compare_and_swap(&dst[v], false, true);
            if (edge_flag[offset+j]){
              edge_data[degree[i]+j] = v;
            }
          } else {
            edge_flag[offset+j] = false;
          } 
        },2048);
      },
      1);  // may need to see the block size
  // rand_seed += non_zeros;
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



size_t BFS::bfs(const sequence<NodeId>& seeds, Hash_Edge _hash_edge) {
  hash_edge = _hash_edge;
  sequence<bool> dst(graph.n);
  parallel_for(0, graph.n, [&](NodeId i) { dst[i] = false; });
  parallel_for(0, seeds.size(), [&](size_t i){
    front[i]=seeds[i];
    dst[seeds[i]]=true;
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

size_t BFS::bfs_sequence(const sequence<NodeId>& seeds, Hash_Edge _hash_edge){
  hash_edge = _hash_edge;
  sequence<bool> dst(graph.n);
  parallel_for(0, graph.n, [&](NodeId i){dst[i]=false;});
  queue<NodeId> Q;
  for (auto s : seeds){
    Q.push(s);
    dst[s]=true;
  }
  size_t cnt = seeds.size();
  while (!Q.empty()){
    NodeId u = Q.front();
    Q.pop();
    for (auto j = graph.offset[u]; j < graph.offset[u+1]; j++){
      NodeId v = graph.E[j];
      // if (hash_edge(u,v,w) && !dst[v]){
      if (hash_edge_id(j,w) && !dst[v]){
        dst[v]=true;
        Q.push(v);
        cnt++;
      }
    }
    // rand_seed+=graph.offset[u+1]-graph.offset[u];
  }
  return cnt;
}

#endif
