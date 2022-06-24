#pragma once

#include <optional>
#include <unordered_set>

#include "get_time.hpp"
#include "graph.hpp"
#include "parlay/sequence.h"
#include "scc.hpp"
#include "hash_bag.h"
#include "tarjan_scc.hpp"
using namespace std;
using namespace parlay;

class PrunedEstimater {
public:
    size_t N;
    Hash_Edge hash_edge;
    Graph DAG;
    sequence<NodeId> comp;    // size N
    sequence<NodeId> weight;  // size n
    sequence<NodeId> sigmas;  // size n
    sequence<bool>  latest;   // size n
    sequence<bool> selected;  // size n

    void first();
    void sigma_first(const sequence<bool>& descendant,  NodeId v_source, bool prune);
// public:
    NodeId sigma(const NodeId v);
    void init_simple(const Graph& graph, size_t graph_id);
    void add(int v);
};


void PrunedEstimater::init_simple(const Graph& graph, size_t graph_id){
    N = graph.n;
    hash_edge.graph_id = (NodeId)graph_id;
    hash_edge.n = graph.n;
    hash_edge.forward = true;
    TARJAN_SCC SCC_P(graph);
    SCC_P.tarjan(hash_edge);
    comp = sequence<NodeId>(N);
    parallel_for (0, N, [&](size_t i){comp[i] = SCC_P.scc[i]-1;});
    size_t n = SCC_P.cnt;
    sequence<edge> edge_list(graph.m);
    sequence<bool> flag(graph.m);
    parallel_for(0, graph.n, [&](size_t i){
        NodeId u = i;
        parallel_for(graph.offset[i], graph.offset[i+1],[&](size_t j){
            NodeId v = graph.E[j];
            float w = graph.W[j];
            if (hash_edge(u,v,w) && comp[u]!= comp[v]){
                flag[j]= true;
                edge_list[j] = edge(comp[u],comp[v]);
            }else{
                flag[j]=false;
            }
        });
    });
    auto dag_edges = pack(edge_list, flag);
    DAG = EdgeListToGraph(dag_edges, n);
    make_directed(DAG);
    weight = sequence<NodeId>(n);
    parallel_for(0, n, [&](size_t i){weight[i]=0;});
    for (size_t i = 0; i< N; i++){
        weight[comp[i]]++;
    }
    first();
    selected = sequence<bool>(n);
}

NodeId PrunedEstimater::sigma(const NodeId v0){
    NodeId v_source = comp[v0];
    if (selected[v_source]){
        return 0;
    }
    if (latest[v_source]){
        return sigmas[v_source];
    }
    NodeId delta = 0;
    sequence<bool> visited(DAG.n);
    parallel_for(0, DAG.n, [&](size_t i){visited[i]=false; });
    queue<NodeId> Q;
    Q.push(v_source);
    visited[v_source] = true;
    while (!Q.empty()){
        NodeId u = Q.front();
        Q.pop();
        delta+= weight[u];
        for (size_t i = DAG.offset[u]; i< DAG.offset[u+1];i++){
            NodeId v = DAG.E[i];
            if (!selected[v] && !visited[v]){
                visited[v]=true;
                Q.push(v);
            }
        }
    }
    sigmas[v_source] = delta;
    latest[v_source]=true;
    return delta;
}

void PrunedEstimater::sigma_first(const sequence<bool>& descendant,  NodeId v_source, bool prune){
    if (latest[v_source]){
        return ;
    }
    NodeId delta = 0;
    sequence<bool> visited(DAG.n);
    parallel_for(0, DAG.n, [&](size_t i){visited[i]=false; });
    queue<NodeId> Q;
    Q.push(v_source);
    visited[v_source] = true;
    while (!Q.empty()){
        NodeId u = Q.front();
        Q.pop();
        delta+= weight[u];
        for (size_t i = DAG.offset[u]; i< DAG.offset[u+1];i++){
            NodeId v = DAG.E[i];
            if (prune && descendant[v]){
                continue;
            }
            if (!visited[v]){
                visited[v]=true;
                Q.push(v);
            }
        }
    }
    sigmas[v_source] = delta;
    latest[v_source] = true;
}

void PrunedEstimater::first(){
    size_t n = DAG.n;
    latest = sequence<bool>(n);
    sigmas = sequence<NodeId>(n);
    
    auto deg_im = parlay::delayed_seq<std::tuple<NodeId,EdgeId>>(n, [&](size_t i){
        EdgeId deg= (DAG.offset[i+1]-DAG.offset[i]) + 
                    (DAG.in_offset[i+1]-DAG.in_offset[i]);
        return make_tuple(i, deg);
    });
    auto red_f = [](const std::tuple<NodeId, EdgeId>& l,
                    const std::tuple<NodeId, EdgeId>& r) {
        return (std::get<1>(l) > std::get<1>(r)) ? l : r;
    };
    auto id = std::make_tuple<NodeId, EdgeId>(0, 0);
    auto monoid = parlay::make_monoid(red_f, id);
    std::tuple<NodeId, EdgeId> sAndD = parlay::reduce(deg_im, monoid);
    NodeId hub = std::get<0>(sAndD);
    parallel_for(0, n, [&](size_t i){latest[i]=false;});
    sequence<bool> descendant(n);
    sequence<bool> ancestor(n);
    parallel_for(0, n, [&](size_t i){
        descendant[i]=false;
        ancestor[i]=false;
    });
    queue<NodeId> Q;
    Q.push(hub);
    NodeId delta = 0;
    descendant[hub]=true;
    for (; ! Q.empty(); ){
        const NodeId u = Q.front();
        Q.pop();
        delta += weight[u];
        for (size_t i = DAG.offset[u]; i != DAG.offset[u+1]; i++){
            const NodeId v = DAG.E[i];
            if (!descendant[v]){
                descendant[v]=true;
                Q.push(v);
            }
        }
    }
    sigmas[hub]=delta;
    latest[hub] = true;
    Q.push(hub);
    ancestor[hub]=true;
    for (; !Q.empty();){
        const NodeId v = Q.front();
        Q.pop();
        for(size_t i = DAG.in_offset[v]; i<DAG.in_offset[v+1]; i++){
            const NodeId u = DAG.in_E[i];
            if (!ancestor[u]){
                ancestor[u]=true;
                Q.push(u);
            }
        }
    }
    // using parallel_for will cause memory usage bloom, becouse of sigma_first
    parallel_for( 0, DAG.n ,[&](size_t i){
        NodeId v = i;
        bool prune = ancestor[v];
        sigma_first(descendant, v, prune);
        if (prune){
            sigmas[v]+= sigmas[hub];
        }
    });
}


void PrunedEstimater::add(int v0){
    NodeId v_add = comp[v0];
    size_t n = DAG.n;
    sequence<bool> visited(n);
    queue<NodeId> Q;
    parallel_for(0, n, [&](size_t i){visited[i]=false;});
    visited[v_add]= true;
    Q.push(v_add);
    hashbag<NodeId> bag(n);
    sequence<NodeId> front(n);
    size_t n_front = 1;
    front[0]=v_add;
    while(!Q.empty()){
        NodeId u = Q.front();
        Q.pop();
        selected[u] = true;
        for (size_t i = DAG.offset[u]; i<DAG.offset[u+1]; i++){
            NodeId v = DAG.E[i];
            if (!selected[v] && !visited[v]){
                Q.push(v);
                visited[v] = true;
                front[n_front]=v;
                latest[v] = false;
                n_front++;
            }
        }
    }
    // for (size_t i = 0; i<n; i++){
    //     if (!selected[i]){
    //         cout << "not empty" << endl;
    //         break;
    //     }
    // }
    while (n_front >0){
        parallel_for(0, n_front, [&](size_t i){
            NodeId u = front[i];
            parallel_for( DAG.in_offset[u], DAG.in_offset[u+1], [&](size_t j){
                NodeId v = DAG.in_E[j];
                if (!selected[v] && !visited[v]){
                    if (latest[v]){
                        if (atomic_compare_and_swap(&latest[v], true, false)){
                            bag.insert(v);
                        }
                    }
                }
            });
        });
        n_front = bag.pack(front);
    }
    
}

