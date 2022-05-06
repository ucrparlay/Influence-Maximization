#pragma once
#include "graph.hpp"
#include "union_find_rules.hpp"


using namespace std;
using namespace parlay;

constexpr size_t TOP_BIT = size_t(UINT_N_MAX) + 1;
constexpr size_t VAL_MASK = UINT_N_MAX;

sequence<size_t> union_find( const Graph &graph, Hash_Edge& hash_edge){
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
  sequence<size_t> parents = sequence<size_t>(graph.n);
  parallel_for(0, graph.n, [&](size_t i){
    parents[i]= (size_t)find(i,label);
  });
  auto hist = histogram_by_key(parents);
  parallel_for(0, hist.size(), [&](size_t i){
    size_t node = hist[i].first;
    size_t size = hist[i].second;
    parents[node] = TOP_BIT | size;
  });
  return parents;
}


template <class Find, typename parent>
struct Unite {
  Find& find;
  Unite(Find& find) : find(find) {}

  inline NodeId operator()(NodeId u_orig, NodeId v_orig,
                           sequence<parent>& parents) {
    parent u = u_orig;
    parent v = v_orig;
    while (1) {
      u = find(u, parents);
      v = find(v, parents);
      if (u == v)
        break;
      else if (u > v && parents[u] == u) {
        parents[u]=v;
        return u;
      } else if (v > u && parents[v] == v) {
        parents[v]=u;
        return v;
      }
    }
    return UINT_N_MAX;
  }
};

sequence<size_t> union_find_serial( const Graph &graph, Hash_Edge& hash_edge){
  auto find = gbbs::find_variants::find_compress;
  auto unite =Unite<decltype(find),NodeId >(find);
  sequence<NodeId> parents = sequence<NodeId>(graph.n);
  for (size_t i = 0; i<graph.n; i++) parents[i]=i;
  for (size_t i = 0; i<graph.n; i++){
    NodeId u = i;
    for (size_t j = graph.offset[i]; j<graph.offset[i+1]; j++){
      NodeId v = graph.E[j];
      float w = graph.W[j];
      if (hash_edge(u,v,w)){
        unite(u,v,parents);
      }
    }
  }
  sequence<size_t> label = sequence<size_t>(graph.n);
  for (size_t i = 0; i<graph.n; i++){label[i]=0;}
  for (size_t i = 0; i<graph.n; i++){
    NodeId p_i = parents[i];
    if (p_i != i){
      label[i]=p_i;
    }
    label[p_i] = ((label[p_i]&VAL_MASK)+1)|TOP_BIT;
  }
  return label;
}