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

sequence<sequence<size_t>> union_find(const Graph& graph, int R){
  auto find = gbbs::find_variants::find_compress;
  auto splice = gbbs::splice_variants::split_atomic_one;
  auto unite =gbbs::unite_variants::UniteRemCAS<decltype(splice), decltype(find),
                                        find_atomic_halve>(find, splice);
  sequence<sequence<NodeId>> labels = sequence<sequence<NodeId>>(R);
  sequence<Hash_Edge> hash_edges = sequence<Hash_Edge>(graph.n);
  for (int i = 0; i<R; i++){
    labels[i] = sequence<NodeId>(graph.n);
    hash_edges[i] = Hash_Edge{(NodeId)i, true};
  }
  
  parallel_for(0, graph.n, [&](size_t i){
    NodeId u = i;
    parallel_for(graph.offset[i], graph.offset[i+1], [&](size_t j){
      NodeId v = graph.E[j];
      float w = graph.W[j];
      parallel_for(0, R, [&](size_t r){
        // Hash_Edge hash_edge = Hash_Edge{(NodeId)i, true};
        if ((hash_edges[r])(u,v,w)){
          unite(u,v,(labels[r]));
        }
      });
    });
  });
  sequence<sequence<size_t>> parents = sequence<sequence<size_t>>(R);
  for (int i = 0; i<R; i++){
    parents[i]=sequence<size_t>(graph.n);
  }
  parallel_for(0, R, [&](size_t r){
    parallel_for(0, graph.n, [&](size_t i){
      parents[r][i] = find(i, labels[r]);
    });
    auto hist = histogram_by_key(parents[r]);
    parallel_for(0, hist.size(), [&](size_t i){
      size_t node = hist[i].first;
      size_t size = hist[i].second;
      parents[r][node] = TOP_BIT | size;
    });
  });
  return parents;
}


// sequence<sequence<size_t>> union_find(const Graph& graph, int R){
//   auto find = gbbs::find_variants::find_compress;
//   auto splice = gbbs::splice_variants::split_atomic_one;
//   auto unite =gbbs::unite_variants::UniteRemCAS<decltype(splice), decltype(find),
//                                         find_atomic_halve>(find, splice);
//   sequence<sequence<NodeId>> labels;
//   for (int i = 0; i<R; i++){
//     labels[i] = sequence<NodeId>(graph.n);
//   }

// }

template <class Find, typename parent>
struct Unite {
  Find& find;
  Unite(Find& find) : find(find) {}

  inline NodeId operator()(NodeId x, NodeId y,
                           sequence<parent>& parents) {
    auto split_atomic_one = [&](NodeId i, NodeId, sequence<parent>& parents){
      parent v = parents[i];
      parent w = parents[v];
      if (v == w)
        return v;
      else {
        parents[i] = w;
        i = w;
        return i;
      }
    };
    NodeId rx = x;
    NodeId ry = y;
    while (parents[rx] != parents[ry]) {
      /* link high -> low */
      parent p_ry = parents[ry];
      parent p_rx = parents[rx];
      if (p_rx < p_ry) {
        std::swap(rx, ry);
        std::swap(p_rx, p_ry);
      }
      // if (rx == parents[rx] && CAS(&parents[rx], rx, p_ry)) {
      if (parents[rx] == parents[rx]){
        parents[rx] = p_ry;
        find(x, parents);
        find(y, parents);
        return rx;
      } else {
        // failure: locally compress by splicing and try again
        rx = split_atomic_one(rx, ry, parents);
      }
    }
    return UINT_N_MAX;
  }
};

sequence<size_t> union_find_serial( const Graph &graph, Hash_Edge& hash_edge){
  auto find = gbbs::find_variants::find_compress;
  auto unite =Unite<decltype(find),NodeId >(find);
  sequence<NodeId> label = sequence<NodeId>(graph.n);
  for (size_t i = 0; i<graph.n; i++) label[i]=i;
  for (size_t i = 0; i<graph.n; i++){
    NodeId u = i;
    for (size_t j = graph.offset[i]; j<graph.offset[i+1]; j++){
      NodeId v = graph.E[j];
      float w = graph.W[j];
      if (hash_edge(u,v,w)){
        unite(u,v,label);
      }
    }
  }
  sequence<size_t> parents = sequence<size_t>(graph.n);
  for (size_t i = 0; i<graph.n; i++){parents[i]=0;}
  for (size_t i = 0; i<graph.n; i++){
    size_t p_i = find(i, label);
    if (p_i != i){
      parents[i]=p_i;
    }
    parents[p_i] = ((parents[p_i]&VAL_MASK)+1)|TOP_BIT;
  }
  return parents;
}