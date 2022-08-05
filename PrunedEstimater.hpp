#pragma once

#include <optional>
#include <unordered_set>

#include "get_time.hpp"
#include "graph.hpp"
#include "parlay/sequence.h"
#include "scc.hpp"
#include "hash_bag.h"
#include "Tarjan_scc.hpp"
using namespace std;
using namespace parlay;

class PrunedEstimater {
public:
    size_t N;
    size_t n_update;
    size_t head, tail;
    NodeId time_stamp;
    Hash_Edge hash_edge;
    Graph DAG;
    sequence<NodeId> comp;    // size N
    sequence<NodeId> weight_offset;  // size n+1
    sequence<NodeId> dcomp;   // size N reverse of comp
    sequence<NodeId> sigmas;  // size n
    sequence<NodeId> update;  // size n
    sequence<NodeId> Q; // size n
    sequence<bool>  latest;   // size n
    sequence<bool> removed;  // size n
    sequence<NodeId> visited;   // size n

    void first();
    NodeId sigma_first(const sequence<bool>& descendant,  NodeId v_source, NodeId prune); //[n]
    inline NodeId unique_child(const NodeId v);
// public:
    NodeId sigma(const NodeId v);   // [n]
    inline NodeId get_sigma(const NodeId v){return sigmas[comp[v]];} //[N]
    void init_simple(const Graph& graph, size_t graph_id);
    void add(NodeId v);
    void scc(const Graph& graph, size_t graph_id);
    sequence<edge> pack_edgelist(const Graph& graph);
    void compute_weight();
    void update_sum(sequence<NodeId>& sum);
    void update_time_stamp(){
        if (time_stamp == UINT_N_MAX){
            parallel_for (0, DAG.n, [&](size_t i){
                visited[i] = 0;
            });
            time_stamp = 1;
        }else{
            time_stamp ++;
        }
    }
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
    comp = sequence<NodeId>(N);
    Tarjan_SCC SCC_P(graph, hash_edge);
    DAG.n = SCC_P.scc(comp);
}


sequence<edge> PrunedEstimater::pack_edgelist(const Graph& graph){
    sequence<edge> edge_list(graph.m);
    sequence<bool> flag(graph.m);
    parallel_for(0, graph.n, [&](size_t i){
        NodeId u = i;
        parallel_for(graph.offset[i], graph.offset[i+1],[&](size_t j){
            NodeId v = graph.E[j];
            float w = graph.W[j];
            if (hash_edge(u,v,w) && (comp[u]!= comp[v])){
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
    removed = sequence<bool>(DAG.n);
    visited = sequence<unsigned int>(DAG.n);
    Q = sequence<NodeId>(DAG.n);
    time_stamp = 0;
    update = sequence<NodeId>(DAG.n);
    parallel_for(0, DAG.n, [&](size_t i){
        removed[i] = false;
        visited[i] = 0;
    });
    first();
}


NodeId PrunedEstimater::sigma(const NodeId v0){
    if (removed[v0]){
        if (!latest[v0]){
            latest[v0]=true;
        }
        sigmas[v0] = 0;
        return 0;
    }
    if (latest[v0]){
        return sigmas[v0];
    }
    latest[v0]=true;
    NodeId child = unique_child(v0);
    NodeId delta = 0;
    if (child == DAG.n) {
        delta = weight_offset[v0+1] - weight_offset[v0];
        if (sigmas[v0] < delta){
            cout << "no child, old: " << sigmas[v0] << " new: " << delta << endl; 
        }
        sigmas[v0]=delta;
        return delta; 
    }
    if (child < DAG.n ){
        delta = sigma(child) + weight_offset[v0+1] - weight_offset[v0];
        if (sigmas[v0] < delta){
            cout << "sketch id " << hash_edge.graph_id << endl;
            cout << "one child, old: " << sigmas[v0] << " new: " << delta << " "; 
            cout << "node " << v0  << " " << weight_offset[v0+1]-weight_offset[v0];
            cout << " child " << child << " sigmas: " <<sigmas[child] <<endl;
            for (size_t i = DAG.offset[v0]; i<DAG.offset[v0+1]; i++){
                cout << "child " << DAG.E[i] << " : " << sigmas[DAG.E[i]] << " latest "<< latest[DAG.E[i]] << endl;
            }
        }
        sigmas[v0]=delta;
        return delta;  
    }
    update_time_stamp();
    head = 0; tail = 0;
    Q[tail++] = v0;
    visited[v0] = time_stamp;
    delta=0;
    while (head != tail){
        NodeId u = Q[head++];
        delta+= (weight_offset[u+1]-weight_offset[u]);
        for (size_t i = DAG.offset[u]; i< DAG.offset[u+1];i++){
            NodeId v = DAG.E[i];
            if ((!removed[v]) && (visited[v] != time_stamp)){
                visited[v]=time_stamp;
                Q[tail++]=v;
            }
        }
    }
    
    if (sigmas[v0] < delta){
        cout << "multiple child, old: " << sigmas[v0] << " new: " << delta;
        cout << " weight: " << weight_offset[v0+1]- weight_offset[v0] << endl; 
        cout << "node: " << v0 << " edge_list: ";
        for (size_t i = DAG.offset[v0]; i<DAG.offset[v0+1]; i++){
            NodeId u = DAG.E[i];
            if (!removed[u]){
                cout << u << " ";
            }
        }
        cout << endl;
    }
    sigmas[v0] = delta;
    return delta;
}

NodeId PrunedEstimater::sigma_first(const sequence<bool>& descendant,  NodeId v_source, NodeId prune){
    if (latest[v_source]){
        return sigmas[v_source];
    }
    latest[v_source] = true;
    EdgeId v_degree = DAG.offset[v_source+1] - DAG.offset[v_source];
    if (v_degree == 0){
        sigmas[v_source] = weight_offset[v_source+1]-weight_offset[v_source];
        return sigmas[v_source];
    }
    if (v_degree == 1){
        NodeId child = DAG.E[DAG.offset[v_source]];
        sigmas[v_source] = weight_offset[v_source+1]-weight_offset[v_source] + sigma_first(descendant, child, prune);
        return sigmas[v_source];
    }
    NodeId delta = 0;
    if (prune){
        delta += prune;
    }
    head = 0;
    tail = 0;
    Q[tail++] = v_source;
    update_time_stamp();
    visited[v_source] = time_stamp;
    while (head != tail){
        NodeId u = Q[head++];
        delta+= (weight_offset[u+1]-weight_offset[u]);
        for (size_t i = DAG.offset[u]; i< DAG.offset[u+1];i++){
            NodeId v = DAG.E[i];
            if (prune && descendant[v]){
                continue;
            }
            if (visited[v] != time_stamp){
                visited[v]=time_stamp;
                Q[tail++] = v;
            }
        }
    }
    sigmas[v_source] = delta;
    return delta;
}

void PrunedEstimater::first(){
    size_t n = DAG.n;
    latest = sequence<bool>(n);
    sigmas = sequence<NodeId>(n);
    
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
    head = 0;
    tail = 0;
    Q[tail++]= hub;
    NodeId delta = 0;
    descendant[hub]=true;
    while (head != tail ){
        const NodeId u = Q[head++];
        delta += (weight_offset[u+1] - weight_offset[u]);
        for (size_t i = DAG.offset[u]; i != DAG.offset[u+1]; i++){
            const NodeId v = DAG.E[i];
            if (!descendant[v]){
                descendant[v]=true;
                Q[tail++]=v;
            }
        }
    }
    sigmas[hub]=delta;
    latest[hub] = true;
    head = 0; tail = 0;
    Q[tail++] = hub;
    ancestor[hub]=true;
    while (head != tail){
        const NodeId v = Q[head];
        head++;
        for(size_t i = DAG.in_offset[v]; i<DAG.in_offset[v+1]; i++){
            const NodeId u = DAG.in_E[i];
            if (!ancestor[u]){
                ancestor[u]=true;
                Q[tail++]=u;
            }
        }
    }
    // using parallel_for will cause memory usage bloom, becouse of sigma_first
    for( size_t i = 0;  i< DAG.n; i++){
        NodeId v = i;
        if ( ancestor[v]){
            sigma_first(descendant, v, delta);
        }else{
            sigma_first(descendant, v, 0);
        }
    }
}


void PrunedEstimater::add(NodeId v_add){
    NodeId v0 = comp[v_add];
    // phase 1, remove points v0 can reach
    head = 0; tail = 0;
    n_update = 0;
    Q[tail++] = v0;
    update[n_update++]=v0;
    removed[v0] = true;
    update_time_stamp();
    visited[v0]=time_stamp;
    while (head != tail){
        NodeId u = Q[head++];
        for (size_t i = DAG.offset[u]; i<DAG.offset[u+1]; i++){
            NodeId v = DAG.E[i];
            if (!removed[v]){
                removed[v] = true;
                Q[tail++]=v;
                update[n_update++]=v;
                latest[v]=false;
                visited[v]=time_stamp;
            }
        }
    }
    // phase 2, push points backward reached to update
    head = 0;
    while (head != tail){
        NodeId u = Q[head++];
        for (size_t i = DAG.in_offset[u]; i<DAG.in_offset[u+1]; i++){
            NodeId v = DAG.in_E[i];
            if ((!removed[v]) && (visited[v]!= time_stamp)){
                visited[v] = time_stamp;
                update[n_update++] = v;
                Q[tail++] = v;
                latest[v] = false;
            }
        }
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
        NodeId new_value = sigma(u);
        for (size_t j = weight_offset[u]; j<weight_offset[u+1]; j++){
            NodeId v = dcomp[j];
            fetch_and_add(&(sum[v]), new_value);   
        }
    }
}

