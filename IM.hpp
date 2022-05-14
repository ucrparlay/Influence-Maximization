#pragma once


#include "graph.hpp"
#include "union_find.hpp"
#include "get_time.hpp"
using namespace std;
using namespace parlay;





class InfluenceMaximizer{
private:
    size_t n,m;
    Graph& G;
    sequence<sequence<size_t>> skeches;
public:
    InfluenceMaximizer() = delete;
    InfluenceMaximizer(Graph & graph):G(graph){
        n = G.n;
        m = G.m;
    }
    void init_sketches(size_t R, int option);
    void init_sketches(size_t R, bool parallel);
    void init_sketches2(size_t R);
    void init_sketches3(size_t R);
    pair<sequence<NodeId>, float> select_seeds(int k, size_t R);
};




void InfluenceMaximizer::init_sketches(size_t R, int option){
    skeches = sequence<sequence<size_t>>(R);
    if (option == 0){
        parallel_for(0, R, [&](size_t i){
            Hash_Edge hash_edge;
            hash_edge.graph_id = i;
            hash_edge.forward = true;
            skeches[i]=union_find(G, hash_edge);
        });
    }else if (option == 1){
        // timer t;
        for (size_t i = 0; i<R; i++){
            Hash_Edge hash_edge;
            hash_edge.graph_id = i;
            hash_edge.forward= true;
            // t.start();
            skeches[i] = union_find(G, hash_edge);
            // cout << "cost: " << t.stop() << endl;
        }
    }else if(option == 2){
        skeches = union_find(G, R);
    }
}

void InfluenceMaximizer::init_sketches2(size_t R) {
  cout << "init_sketches2" << endl;
  auto find = gbbs::find_variants::find_compress;
  auto splice = gbbs::splice_variants::split_atomic_one;
  auto unite =
      gbbs::unite_variants::UniteRemCAS<decltype(splice), decltype(find),
                                        find_atomic_halve>(find, splice);
  sequence<sequence<NodeId>> label(R, sequence<NodeId>(G.n));
  parallel_for(0, R, [&](size_t i) {
    parallel_for(0, G.n, [&](size_t j) { label[i][j] = j; });
  });
  sequence<Hash_Edge> hash_edges(R);
  for (size_t i = 0; i < R; i++) {
    hash_edges[i].graph_id = i;
    hash_edges[i].forward = true;
  }
  parallel_for(0, G.n, [&](size_t i) {
    NodeId u = i;
    parallel_for(G.offset[i], G.offset[i + 1], [&](size_t j) {
      NodeId v = G.E[j];
      float w = G.W[j];
      parallel_for(0, R, [&](size_t k) {
        if (hash_edges[k](u, v, w)) {
        // if (rand() < RAND_MAX * w) {
          unite(u, v, label[k]);
        }
      });
    });
  });
  skeches = sequence<sequence<size_t>>(R, sequence<size_t>(G.n));
  parallel_for(0, R, [&](size_t k) {
    auto& parents = skeches[k];
    parallel_for(0, G.n,
                 [&](size_t i) { parents[i] = (size_t)find(i, label[k]); });
    auto hist = histogram_by_key(parents);
    parallel_for(0, hist.size(), [&](size_t i) {
      size_t node = hist[i].first;
      size_t size = hist[i].second;
      parents[node] = TOP_BIT | size;
    });
  });
}

void InfluenceMaximizer::init_sketches3(size_t R) {
  cout << "init_sketches3" << endl;
  auto find = gbbs::find_variants::find_compress;
  auto splice = gbbs::splice_variants::split_atomic_one;
  auto unite =
      gbbs::unite_variants::UniteRemCAS<decltype(splice), decltype(find),
                                        find_atomic_halve>(find, splice);
  sequence<sequence<NodeId>> label(R, sequence<NodeId>(G.n));
  parallel_for(0, R, [&](size_t i) {
    parallel_for(0, G.n, [&](size_t j) { label[i][j] = j; });
  });
  sequence<Hash_Edge> hash_edges(R);
  for (size_t i = 0; i < R; i++) {
    hash_edges[i].graph_id = i;
    hash_edges[i].forward = true;
  }
  parallel_for(0, G.n, [&](size_t i) {
    NodeId u = i;
    parallel_for(G.offset[i], G.offset[i + 1], [&](size_t j) {
      NodeId v = G.E[j];
      float w = G.W[j];
      parallel_for(0, R * w + 2, [&](size_t k) {
        auto t = _hash(_hash(u)+v)+_hash(k);
        auto id = t % R;
        unite(u, v, label[id]);
      });
    });
  });
  skeches = sequence<sequence<size_t>>(R, sequence<size_t>(G.n));
  parallel_for(0, R, [&](size_t k) {
    auto& parents = skeches[k];
    parallel_for(0, G.n,
                 [&](size_t i) { parents[i] = (size_t)find(i, label[k]); });
    auto hist = histogram_by_key(parents);
    parallel_for(0, hist.size(), [&](size_t i) {
      size_t node = hist[i].first;
      size_t size = hist[i].second;
      parents[node] = TOP_BIT | size;
    });
  });
}

pair<sequence<NodeId>,float> InfluenceMaximizer::select_seeds(int k, size_t R){
    timer t;
    t.start();
    init_sketches(R, true);
    t.stop();
    cout << "initial sketches: " << t.get_total() << endl;
    float influence_sum=0;
    sequence<NodeId> seeds(k);
    for (int t = 0; t<k; t++){
        sequence<size_t> influence(G.n);
        parallel_for(0, G.n, [&](size_t i){
            influence[i]=0;
            for (size_t r = 0; r<R; r++){
                size_t p_i = skeches[r][i];
                if (!(p_i & TOP_BIT)){
                    p_i = skeches[r][p_i];
                }
                influence[i] += (p_i &VAL_MASK); 
            }
        });
        auto max_influence = parlay::max_element(influence);
        NodeId seed = max_influence - influence.begin();
        float influence_gain = influence[seed]/(R+0.0);
        influence_sum+=influence_gain;
        // cout << "seed " << t << ": " << seed << " spread " <<influence_gain << endl;
        seeds[t]=seed;
        parallel_for(0, R, [&](size_t r){
            size_t p_seed = skeches[r][seed];
            if (!(p_seed & TOP_BIT)){
                skeches[r][p_seed] = TOP_BIT | 0;
            }else{
                skeches[r][seed] = TOP_BIT | 0;
            }
        });
    }
    return make_pair(seeds,influence_sum);
}