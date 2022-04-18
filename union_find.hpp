#pragma once
#include "graph.hpp"
#include "union_find_rules.hpp"

using namespace std;
using namespace parlay;

sequence<size_t> union_find( const Graph &graph, Hash_Edge& hash_edge){
  size_t TOP_BIT = (size_t)UINT_N_MAX + 1;
  size_t VAL_MASK = (size_t) UINT_N_MAX;
  auto find = gbbs::find_variants::find_compress;
  auto splice = gbbs::splice_variants::split_atomic_one;
  auto unite =gbbs::unite_variants::UniteRemCAS<decltype(splice), decltype(find),
                                        find_atomic_halve>(find, splice);
  sequence<NodeId> label = sequence<NodeId>(graph.n);
  parallel_for(0, graph.n, [&](size_t i){
    label[i]= i;
  });
  parallel_for(0, graph.n, [&](size_t i){
    NodeId u = i;
    parallel_for(graph.offset[i], graph.offset[i+1], [&](size_t j){
      NodeId v = graph.E[j];
      float w = graph.W[j];
      if (hash_edge(u,v,w)){
        unite(u,v,label);
      }
    });
  });

  sequence<size_t> parent = sequence<size_t>(graph.n);
  parallel_for(0, graph.n, [&](size_t i){
      parent[i]= TOP_BIT | 0;
  });
  parallel_for(0, graph.n,[&](size_t i){
    NodeId p_i = find(i, label);
    if (p_i != i){
        parent[i]=(size_t)p_i;
    }
    size_t oldV, newV;
    do {
        oldV = parent[p_i];
        newV = ((oldV & VAL_MASK)+1) | TOP_BIT;
    }while (!CAS(&parent[p_i], oldV, newV));
  });
  return parent;
}