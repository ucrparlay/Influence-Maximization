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
    sequence<sequence<size_t>> sketches;
public:
    InfluenceMaximizer() = delete;
    InfluenceMaximizer(Graph & graph):G(graph){
        n = G.n;
        m = G.m;
    }
    void init_sketches(size_t R, int option);
    // void init_sketches(size_t R, bool parallel);
    void init_sketches2(size_t R);
    void init_sketches3(size_t R);
    sequence<pair<NodeId, float>> select_seeds(int k, size_t R, bool CELF);
};




void InfluenceMaximizer::init_sketches(size_t R, int option){
    sketches = sequence<sequence<size_t>>(R, sequence<size_t>(G.n));
    if (option == 0){
        parallel_for(0, R, [&](size_t i){
            Hash_Edge hash_edge;
            hash_edge.graph_id = i;
            hash_edge.forward = true;
            union_find(G, hash_edge, sketches[i]);
        });
    }else if (option == 1){
        // timer t;
        for (size_t i = 0; i<R; i++){
            Hash_Edge hash_edge;
            hash_edge.graph_id = i;
            hash_edge.forward= true;
            // t.start();
            union_find(G, hash_edge, sketches[i]);
            // cout << "cost: " << t.stop() << endl;
        }
    }else if(option == 2){
        union_find(G, R, sketches);
    }
}

void InfluenceMaximizer::init_sketches2(size_t R) {
  cout << "init_sketches2" << endl;
  auto find = gbbs::find_variants::find_compress;
  auto splice = gbbs::splice_variants::split_atomic_one;
  auto unite =
      gbbs::unite_variants::UniteRemCAS<decltype(splice), decltype(find),
                                        find_atomic_halve>(find, splice);
  timer t;
  t.start();
  sequence<sequence<NodeId>> label(R, sequence<NodeId>(G.n));
  parallel_for(0, R, [&](size_t i) {
    parallel_for(0, G.n, [&](size_t j) { label[i][j] = j; });
  });
  sequence<Hash_Edge> hash_edges(R);
  for (size_t i = 0; i < R; i++) {
    hash_edges[i].graph_id = i;
    hash_edges[i].forward = true;
  }
  cout << "initialize sketches: " << t.stop() << endl;
  t.start();
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
  cout << "union find: " << t.stop() << endl;
  t.start();
  sketches = sequence<sequence<size_t>>(R, sequence<size_t>(G.n));
  parallel_for(0, R, [&](size_t k) {
    auto& parents = sketches[k];
    parallel_for(0, G.n,
                 [&](size_t i) { parents[i] = (size_t)find(i, label[k]); });
    auto hist = histogram_by_key(parents);
    parallel_for(0, hist.size(), [&](size_t i) {
      size_t node = hist[i].first;
      size_t size = hist[i].second;
      parents[node] = TOP_BIT | size;
    });
  });
  cout << "after union_find " <<  t.stop() << endl;
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
  sketches = sequence<sequence<size_t>>(R, sequence<size_t>(G.n));
  parallel_for(0, R, [&](size_t k) {
    auto& parents = sketches[k];
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

sequence<pair<NodeId,float>> InfluenceMaximizer::select_seeds(int k, size_t R, bool CELF){
    timer t;
    sequence<pair<NodeId,float>> seeds(k);
    if (k ==0) return seeds;
    sequence<size_t> influence(G.n);
    // --------begin round 1 ---------
    parallel_for(0, G.n, [&](size_t i){
            influence[i]=0;
            for (size_t r = 0; r<R; r++){
                size_t p_i = sketches[r][i];
                if (!(p_i & TOP_BIT)){
                    p_i = sketches[r][p_i];
                }
                influence[i] += (p_i & VAL_MASK); 
            }
    },1024);
    auto max_influence_it = parlay::max_element(influence);
    NodeId seed = max_influence_it - influence.begin();
    float influence_gain = influence[seed]/(R+0.0);
    seeds[0]=make_pair(seed, influence_gain);
    parallel_for(0, R, [&](size_t r){
            size_t p_seed = sketches[r][seed];
            if (!(p_seed & TOP_BIT)){
                sketches[r][p_seed] = TOP_BIT | 0;
            }else{
                sketches[r][seed] = TOP_BIT | 0;
            }
        });
    // ----------end round 1 ------------
    for (int t = 1; t<k; t++){
        size_t max_influence = 0;
        parallel_for(0, G.n, [&](size_t i){
            size_t new_influence = 0;
            if ((!CELF) || influence[i] >= max_influence){
              for (size_t r = 0; r<R; r++){
                size_t p_i = sketches[r][i];
                  if (!(p_i & TOP_BIT)){
                      p_i = sketches[r][p_i];
                  }
                  new_influence += (p_i & VAL_MASK); 
              }
              influence[i] = new_influence;
            }
            if (CELF && (new_influence > max_influence)){
              write_max(&max_influence, new_influence, [](size_t a, size_t b){return a<b;});
            }
        },1024);
        if (CELF){
          max_influence_it = parlay::find(make_slice(influence), max_influence);
        }else{
          max_influence_it = parlay::max_element(influence);
        }
        seed = max_influence_it - influence.begin();
        influence_gain = max_influence/(R+0.0);
        seeds[t]=make_pair(seed, influence_gain);
        parallel_for(0, R, [&](size_t r){
            size_t p_seed = sketches[r][seed];
            if (!(p_seed & TOP_BIT)){
                sketches[r][p_seed] = TOP_BIT | 0;
            }else{
                sketches[r][seed] = TOP_BIT | 0;
            }
        });
    }
    return seeds;
}