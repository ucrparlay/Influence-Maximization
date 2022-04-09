#pragma once


#include "graph.hpp"
#include "union_find_rules.hpp"
using namespace std;
using namespace parlay;


struct CC{
    int n;
    sequence<NodeId> label; 
    sequence<NodeId> size;
};




class InfluenceMaximizer{
private:
    size_t n,m;
    Graph& G;
    sequence<CC> skeches;
    void uniform_weight(float w);  // TODO: read from Graph
    void init_sketches(size_t R);
public:
    InfluenceMaximizer() = delete;
    InfluenceMaximizer(Graph & graph):G(graph){
        n = G.n;
        m = G.m;
        uniform_weight(0.1);
    }
    bool check_edge(EdgeId j, size_t sketch_id);
    CC connect(size_t sketch_id);
    sequence<NodeId> seeds(int k, size_t R);
};

void InfluenceMaximizer::uniform_weight(float w){
    G.W = sequence<float>(m);
    // G.in_W = sequence<float>(m);
    parallel_for(0, n, [&](size_t i){
        G.W[i]=w;
        // G.in_W[i]=w;
    });
}

bool InfluenceMaximizer::check_edge(EdgeId e_id, size_t sketch_id){
    float p = (_hash(e_id) + _hash<EdgeId>(sketch_id)+0.0)/UINT_E_MAX;
    return p<G.W[e_id];
}


CC InfluenceMaximizer::connect(size_t sketch_id){
    auto find = gbbs::find_variants::find_compress;
    auto splice = gbbs::splice_variants::split_atomic_one;
    auto unite =
        gbbs::unite_variants::UniteRemCAS<decltype(splice), decltype(find),
                                        find_atomic_halve>(find, splice);
    CC cc;
    cc.n=G.n;
    cc.label=sequence<NodeId>(G.n);
    cc.size=sequence<NodeId>(G.n);
    parallel_for(0,G.n,[&](NodeId i){
        cc.label[i]=i;
        cc.size[i]=0;
    });
    parallel_for(0, G.n,[&](NodeId i) {
        parallel_for(G.offset[i], G.offset[i + 1],[&](EdgeId j) {
            NodeId v = G.E[j];
            if (check_edge(j, sketch_id)) {
                unite(i, v, cc.label);
            }
        },BLOCK_SIZE);
    },512);

    parallel_for(0,G.n, [&](NodeId i){
        cc.label[i]=find(cc.label[i],cc.label);
        fetch_and_add(&cc.size[cc.label[i]], 1);
    });
    return cc;
};

void InfluenceMaximizer::init_sketches(size_t R){
    skeches = sequence<CC>(R);
    parallel_for(0, R, [&](size_t i){
        skeches[i]=connect(i);
    });
}

sequence<NodeId> InfluenceMaximizer::seeds(int k, size_t R){
    sequence<NodeId> select_seeds(k);
    init_sketches(R);
    for (int t = 0; t<k; t++){
        float max_influence=0;
        NodeId seed=UINT_N_MAX; 
        for (NodeId v = 0; v<G.n; v++){
            float gain=0;
            for (size_t r = 0; r<R; r++){
                NodeId label = skeches[r].label[v];
                gain+=skeches[r].size[label];
            }
            gain = gain/R;
            if (gain>max_influence){
                max_influence = gain;
                seed = v;
            }
        }
        select_seeds[t]=seed;
        for (size_t r = 0; r<R; r++){
            NodeId label = skeches[r].label[seed];
            skeches[r].size[label]=0;
        }
        cout << "seed " << t << ": " << seed << endl;
    }
    return select_seeds;
}