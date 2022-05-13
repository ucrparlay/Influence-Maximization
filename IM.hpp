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
    void init_sketches(size_t R, bool parallel);
    pair<sequence<NodeId>, float> select_seeds(int k, size_t R);
};




void InfluenceMaximizer::init_sketches(size_t R, bool parallel){
    skeches = sequence<sequence<size_t>>(R);
    if (parallel){
        parallel_for(0, R, [&](size_t i){
            Hash_Edge hash_edge;
            hash_edge.graph_id = i;
            hash_edge.forward = true;
            skeches[i]=union_find(G, hash_edge);
        });
    }else{
        for (size_t i = 0; i<R; i++){
            Hash_Edge hash_edge;
            hash_edge.graph_id = i;
            hash_edge.forward= true;
            skeches[i] = union_find(G, hash_edge);
        }
    }
}

pair<sequence<NodeId>,float> InfluenceMaximizer::select_seeds(int k, size_t R){
    timer t;
    t.start();
    init_sketches(R, true);
    t.stop();
    // cout << "initial sketches: " << t.get_total() << endl; 
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