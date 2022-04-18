#pragma once


#include "graph.hpp"
#include "union_find.hpp"
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
    sequence<size_t> connect(size_t sketch_id);
    void init_sketches(size_t R);
    // sequence<NodeId> seeds(int k, size_t R);
};



sequence<size_t> InfluenceMaximizer::connect(size_t sketch_id){
    Hash_Edge hash_edge;
    hash_edge.graph_id=sketch_id;
    auto label = union_find(G, hash_edge);
    return label;
};

void InfluenceMaximizer::init_sketches(size_t R){
    skeches = sequence<sequence<size_t>>(R);
    parallel_for(0, R, [&](size_t i){
        skeches[i]=connect(i);
    });
}

// sequence<NodeId> InfluenceMaximizer::seeds(int k, size_t R){
//     sequence<NodeId> select_seeds(k);
//     init_sketches(R);
//     for (int t = 0; t<k; t++){
//         float max_influence=0;
//         NodeId seed=UINT_N_MAX; 
//         for (NodeId v = 0; v<G.n; v++){
//             float gain=0;
//             for (size_t r = 0; r<R; r++){
//                 NodeId label = skeches[r].label[v];
//                 gain+=skeches[r].size[label];
//             }
//             gain = gain/R;
//             if (gain>max_influence){
//                 max_influence = gain;
//                 seed = v;
//             }
//         }
//         select_seeds[t]=seed;
//         for (size_t r = 0; r<R; r++){
//             NodeId label = skeches[r].label[seed];
//             skeches[r].size[label]=0;
//         }
//         cout << "seed " << t << ": " << seed << endl;
//     }
//     return select_seeds;
// }