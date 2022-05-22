#pragma once
#include "graph.hpp"
#include "union_find_rules.hpp"


using namespace std;
using namespace parlay;

constexpr size_t TOP_BIT = size_t(UINT_N_MAX) + 1;
constexpr size_t VAL_MASK = UINT_N_MAX;

void union_find( const Graph &graph, Hash_Edge& hash_edge, sequence<size_t>& parents){
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
  // sequence<size_t> parents = sequence<size_t>(graph.n);
  parallel_for(0, graph.n, [&](size_t i){
    parents[i]= (size_t)find(i,label);
  });
  auto hist = histogram_by_key(parents);
  parallel_for(0, hist.size(), [&](size_t i){
    size_t node = hist[i].first;
    size_t size = hist[i].second;
    parents[node] = TOP_BIT | size;
  });
  // return parents;
}

void union_find(const Graph& graph, int R, sequence<sequence<size_t>>& sketches){
  auto find = gbbs::find_variants::find_compress;
  auto splice = gbbs::splice_variants::split_atomic_one;
  auto unite =gbbs::unite_variants::UniteRemCAS<decltype(splice), decltype(find),
                                        find_atomic_halve>(find, splice);
  timer t;
  t.start();
  sequence<sequence<NodeId>> labels(R, sequence<NodeId>(graph.n));
  sequence<Hash_Edge> hash_edges(R);

  parallel_for(0, R, [&](size_t r){
    hash_edges[r].graph_id = r;
    hash_edges[r].forward = true;
    parallel_for(0, graph.n, [&](size_t i){
      labels[r][i] = i;
    });
  });
  t.stop();
  cout << "initial sketches: " << t.stop() << endl;
  t.start();
  parallel_for(0, graph.n, [&](size_t i){
    NodeId u = i;
    parallel_for(graph.offset[i], graph.offset[i+1], [&](size_t j){
      NodeId v = graph.E[j];
      float w = graph.W[j];
      parallel_for(0, R, [&](size_t r){
        // Hash_Edge hash_edge = Hash_Edge{(NodeId)i, true};
        if (hash_edges[r](u,v,w)){
          unite(u,v,labels[r]);
        }
      });
    });
  });
  cout << "union find: " << t.stop() << endl;
  // sketches = sequence<sequence<size_t>> (R,sequence<size_t>(graph.n));
  t.start();
  parallel_for(0, R, [&](size_t r){
    parallel_for(0, graph.n, [&](size_t i){
      sketches[r][i] = find(i, labels[r]);
    }, 1024);
    // --------sort & pack begin----------------
    // timer t_hist;
    // t_hist.start();
    // auto sorted_lable = sort(sketches[r]);
    // double sort_cost = t_hist.stop();
    // t_hist.start();
    // sequence<bool> flag(graph.n);
    // parallel_for(1, graph.n, [&](size_t i){
    //   if (sorted_lable[i]!= sorted_lable[i-1]){
    //     flag[i-1]=true;
    //   }else{
    //     flag[i-1]=false;
    //   }
    // },1024);
    // flag[graph.n-1] = true;
    // auto unique_labels = pack(make_slice(sorted_lable), flag);
    // auto sizes_scan = pack_index(flag);
    // sketches[r][unique_labels[0]] = TOP_BIT | sizes_scan[0];
    // parallel_for(1, sizes_scan.size(), [&](size_t i){
    //   auto sizes = sizes_scan[i]-sizes_scan[i-1];
    //   sketches[r][unique_labels[i]] = TOP_BIT | sizes;
    // },1024);
    // double pack_cost = t_hist.stop();
    // printf("sort_cost %f, pack_cost %f\n", sort_cost, pack_cost);
    // --------sort & pack end------------------
    // --------hist begin-----------------------
    // auto hist = histogram_by_key(sketches[r]);
    // using Hash = decltype(std::hash<size_t>());
    // using Equal = decltype(equal_to<size_t>());
    // auto helper = count_by_key_helper<size_t,size_t,Hash,Equal>{std::hash<size_t>(),equal_to<size_t>()};
    // // auto hist = internal::seq_collect_reduce_sparse<uninitialized_relocate_tag>(make_slice(sketches[r]), helper);
    // parallel_for(0, hist.size(), [&](size_t i){
    //   size_t node = hist[i].first;
    //   size_t size = hist[i].second;
    //   sketches[r][node] = TOP_BIT | size;
    // --------hist end---------------------------
    // --------sequential scan begin -------------
    sequence<NodeId> hist(graph.n);
    parallel_for(0, graph.n, [&](size_t i){hist[i] = 0;});
    for (size_t i = 0; i<graph.n; i++){
      NodeId p_label = sketches[r][i];
      hist[p_label]++;
    }
    // auto max_cc = parlay::max_element(hist);
    // cout << "max_cc " << *max_cc << endl;
    parallel_for(0, hist.size(), [&](size_t i){
      if (hist[i]!= 0){
        sketches[r][i] = TOP_BIT | (size_t) hist[i];
      }
    });
    // --------sequential scan end -----------------
  });
  cout << "after union_find: " << t.stop() << endl;
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