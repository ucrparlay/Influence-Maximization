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
    size_t n_update;
    Hash_Edge hash_edge;
    Graph DAG;
    sequence<NodeId> comp;    // size N
    sequence<NodeId> weight_offset;  // size n+1
    sequence<NodeId> dcomp;   // size N reverse of comp
    sequence<NodeId> sigmas;  // size n
    sequence<NodeId> update;  // size n
    sequence<bool>  latest;   // size n
    sequence<bool> removed;  // size n
    sequence<bool> visited;   // size n

    void first();
    void sigma_first(const sequence<bool>& descendant,  NodeId v_source, NodeId prune);
    inline NodeId unique_child(const NodeId v);
// public:
    NodeId sigma(const NodeId v);
    bool IsLatest(const NodeId v){return latest[comp[v]];}
    NodeId GetSigmas(const NodeId v){return sigmas[comp[v]];}
    void init_simple(const Graph& graph, size_t graph_id);
    void add(int v);
    void scc(const Graph& graph, size_t graph_id);
    sequence<edge> pack_edgelist(const Graph& graph);
    void compute_weight();
    void update_sum(sequence<NodeId>& sum);
};

inline NodeId PrunedEstimater::unique_child(const NodeId v){
    NodeId out_deg = 0;
    NodeId child;
    for (size_t i = DAG.offset[v]; i<DAG.offset[v+1]; i++){
        const NodeId u = DAG.E[i];
        if (!removed[u]){
            out_deg ++;
            child = u;
        }
    }
    if (out_deg == 0){
        return DAG.n;
    } else if (out_deg ==1){
        return child;
    }else{
        return DAG.n+1;
    }
}

void PrunedEstimater::scc(const Graph& graph, size_t graph_id){
    N = graph.n;
    hash_edge.graph_id = (NodeId)graph_id;
    hash_edge.n = graph.n;
    hash_edge.forward = true;
    TARJAN_SCC SCC_P(graph);
    SCC_P.tarjan(hash_edge);
    comp = sequence<NodeId>(N);
    parallel_for (0, N, [&](size_t i){comp[i] = SCC_P.scc[i]-1;});
    DAG.n = SCC_P.cnt;
}


sequence<edge> PrunedEstimater::pack_edgelist(const Graph& graph){
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
    return dag_edges;
}
void PrunedEstimater::compute_weight(){
    sequence<edge> edge_list(N);
    parallel_for(0, N, [&](size_t i){
        edge_list[i]=edge(comp[i], i);
    });
    sort_inplace(edge_list);
    weight_offset = sequence<NodeId>(DAG.n+1);
    dcomp = sequence<NodeId>(N);
    parallel_for(0, N, [&](size_t i){
        NodeId u = edge_list[i].u;  //scc label
        NodeId v = edge_list[i].v;
        dcomp[i]= v;
        if (i == 0 || edge_list[i-1].u != u){
            weight_offset[u] = i;
        }
    });
    weight_offset[DAG.n] = N;
}

void PrunedEstimater::init_simple(const Graph& graph, size_t graph_id){
    scc(graph, graph_id);
    auto dag_edges = pack_edgelist(graph);
    DAG = EdgeListToGraph(dag_edges, DAG.n);
    make_directed(DAG);
    compute_weight();
    first();
    removed = sequence<bool>(DAG.n);
    visited = sequence<bool>(DAG.n);
    parallel_for(0, DAG.n, [&](size_t i){
        removed[i] = false;
        visited[i] = false;
    });
}


NodeId PrunedEstimater::sigma(const NodeId v0){
    NodeId v_source = comp[v0];
    if (removed[v_source]){
        if (!latest[v_source]){
            latest[v_source]=true;
        }
        return 0;
    }
    if (latest[v_source]){
        return sigmas[v_source];
    }
    latest[v_source]=true;
    NodeId child = unique_child(v_source);
    if (child == DAG.n) return sigmas[v_source] =  weight_offset[v_source+1] - weight_offset[v_source];
    if (child < DAG.n ) return sigmas[v_source] =  sigma(child) + weight_offset[v_source+1] - weight_offset[v_source];
    NodeId delta = 0;
    parallel_for(0, DAG.n, [&](size_t i){visited[i]=false; });
    queue<NodeId> Q;
    Q.push(v_source);
    visited[v_source] = true;
    while (!Q.empty()){
        NodeId u = Q.front();
        Q.pop();
        delta+= (weight_offset[u+1]-weight_offset[u]);
        for (size_t i = DAG.offset[u]; i< DAG.offset[u+1];i++){
            NodeId v = DAG.E[i];
            if (!removed[v] && !visited[v]){
                visited[v]=true;
                Q.push(v);
            }
        }
    }
    sigmas[v_source] = delta;
    return delta;
}

void PrunedEstimater::sigma_first(const sequence<bool>& descendant,  NodeId v_source, NodeId prune){
    if (latest[v_source]){
        return ;
    }
    latest[v_source] = true;
    NodeId delta = 0;
    if (prune){
        delta += prune;
    }
    // parallel_for(0, DAG.n, [&](size_t i){visited[i]=false; });
    EdgeId v_degree = DAG.offset[v_source+1] - DAG.offset[v_source];
    if (v_degree == 0){
        sigmas[v_source] = weight_offset[v_source+1]-weight_offset[v_source];
        return;
    }
    if (v_degree == 1){
        NodeId child = DAG.E[DAG.offset[v_source]];
        sigma_first(descendant, child, prune);
        sigmas[v_source] = weight_offset[v_source+1]-weight_offset[v_source] + sigmas[v_source];
        return;
    }
    queue<NodeId> Q;
    Q.push(v_source);
    visited[v_source] = true;
    // use update as a vector now;
    n_update = 1;
    update[0] = v_source;
    while (!Q.empty()){
        NodeId u = Q.front();
        Q.pop();
        delta+= (weight_offset[u+1]-weight_offset[u]);
        for (size_t i = DAG.offset[u]; i< DAG.offset[u+1];i++){
            NodeId v = DAG.E[i];
            if (prune && descendant[v]){
                continue;
            }
            if (!visited[v]){
                visited[v]=true;
                update[n_update++] = v;
            }
        }
    }
    for (size_t i = 0; i<n_update; i++){
        visited[update[i]] = false;
    }
    sigmas[v_source] = delta;
}

void PrunedEstimater::first(){
    size_t n = DAG.n;
    latest = sequence<bool>(n);
    sigmas = sequence<NodeId>(n);
    
    // auto deg_im = parlay::delayed_seq<std::tuple<NodeId,EdgeId>>(n, [&](size_t i){
    //     EdgeId deg= (DAG.offset[i+1]-DAG.offset[i]) + 
    //                 (DAG.in_offset[i+1]-DAG.in_offset[i]);
    //     return make_tuple(i, deg);
    // });
    // auto red_f = [](const std::tuple<NodeId, EdgeId>& l,
    //                 const std::tuple<NodeId, EdgeId>& r) {
    //     return (std::get<1>(l) > std::get<1>(r)) ? l : r;
    // };
    // auto id = std::make_tuple<NodeId, EdgeId>(0, 0);
    // auto monoid = parlay::make_monoid(red_f, id);
    // std::tuple<NodeId, EdgeId> sAndD = parlay::reduce(deg_im, monoid);
    // NodeId hub = std::get<0>(sAndD);
    NodeId hub = 0;
    EdgeId max_degree_sum = DAG.offset[1] + DAG.in_offset[1];
    for (size_t i = 1; i<DAG.n; i++){
        if (DAG.offset[i+1]-DAG.offset[i] + DAG.in_offset[i+1]-DAG.in_offset[i] 
        > max_degree_sum){
            hub = i;
            max_degree_sum = DAG.offset[i+1]-DAG.offset[i] + DAG.in_offset[i+1]-DAG.in_offset[i];
        }
    }
    sequence<bool> descendant(n);
    sequence<bool> ancestor(n);
    parallel_for(0, n, [&](size_t i){
        latest[i] = false;
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
        delta += (weight_offset[u+1] - weight_offset[u]);
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
    parallel_for(0, DAG.n, [&](size_t i){visited[i]=false;});
    for( size_t i = 0;  i< DAG.n; i++){
        NodeId v = i;
        if ( ancestor[v]){
            sigma_first(descendant, v, delta);
        }else{
            sigma_first(descendant, v, 0);
        }
    }
}


void PrunedEstimater::add(int v0){
    NodeId v_add = comp[v0];
    size_t n = DAG.n;
    queue<NodeId> Q;
    parallel_for(0, n, [&](size_t i){visited[i]=false;});
    visited[v_add]= true;
    Q.push(v_add);
    hashbag<NodeId> bag(n);
    sequence<NodeId> front(n);  // can merge with sequence update
    size_t n_front = 1;
    front[0]=v_add;
    update[0]=v_add;
    n_update = 1;
    while(!Q.empty()){
        NodeId u = Q.front();
        Q.pop();
        removed[u] = true;
        for (size_t i = DAG.offset[u]; i<DAG.offset[u+1]; i++){
            NodeId v = DAG.E[i];
            if (!removed[v] && !visited[v]){
                Q.push(v);
                visited[v] = true;
                front[n_front]=v;
                latest[v] = false;
                update[n_update]=v;
                n_update++;
                n_front++;
            }
        }
    }
    while (n_front >0){
        parallel_for(0, n_front, [&](size_t i){
            NodeId u = front[i];
            parallel_for( DAG.in_offset[u], DAG.in_offset[u+1], [&](size_t j){
                NodeId v = DAG.in_E[j];
                if (!removed[v] && !visited[v]){
                    if (latest[v]){
                        if (atomic_compare_and_swap(&latest[v], true, false)){
                            update[n_update]=v;
                            n_update++;
                            bag.insert(v);
                        }
                    }
                }
            });
        });
        n_front = bag.pack(front);
    }
    
}
void PrunedEstimater::update_sum(sequence<NodeId>& sum){
    for (size_t i = 0; i<n_update; i++){
        NodeId u = update[i];
        NodeId old_value = sigmas[u];
        for (size_t j = weight_offset[u]; j<weight_offset[u+1]; j++){
            NodeId v = dcomp[j];
            fetch_and_minus(&(sum[v]), old_value);
        }
    }
    for (size_t i = 0; i<n_update; i++){
        NodeId u = update[i];
        sigma(u);
        NodeId new_value = sigmas[u];
        for (size_t j = weight_offset[u]; j<weight_offset[u+1]; j++){
            NodeId v = dcomp[j];
            fetch_and_add(&(sum[v]), new_value);   
        }
    }
}

