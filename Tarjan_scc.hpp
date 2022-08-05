#pragma once
#include <parlay/random.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <stack>

#include "get_time.hpp"
#include "graph.hpp"
#include "utilities.h"

using namespace std;

class Tarjan_SCC{
  private:
    const Graph& G;
    const Hash_Edge& hash_edge;
    sequence<NodeId> stack;
    sequence<NodeId> call_stack;
    sequence<NodeId> low;
    sequence<NodeId> dfn;
    sequence<EdgeId> edge_offset;
    size_t stack_head, call_head;
    size_t idx,cnt;
    void dfs(NodeId u, sequence<NodeId>& label);
  public:
    size_t scc(sequence<NodeId>& label);
    Tarjan_SCC()=delete;
    Tarjan_SCC(const Graph& graph, const Hash_Edge& _hash_edge)
                :G(graph), hash_edge(_hash_edge){
        low = sequence<NodeId>(G.n);
        dfn = sequence<NodeId>(G.n);
        stack = sequence<NodeId>(G.n);
        call_stack = sequence<NodeId>(G.n);
        edge_offset = sequence<EdgeId>(G.n);
    }
};

void Tarjan_SCC::dfs(NodeId _u, sequence<NodeId>& label){
    call_stack[call_head++]=_u;
    while (call_head!=0){
        NodeId u = call_stack[call_head-1];
        EdgeId begin_offset; 
        if (dfn[u]==0){
            low[u]=dfn[u] = ++idx;
            stack[stack_head++]=u;
            begin_offset=G.offset[u];
        }else{
            NodeId back_from = G.E[edge_offset[u]];
            low[u]=min(low[u],low[back_from]);
            begin_offset = edge_offset[u]+1;
        }
        bool breakdown = false;
        for (; begin_offset<G.offset[u+1];begin_offset++){
            NodeId v = G.E[begin_offset];
            float w = G.W[begin_offset];
            if (!hash_edge(u,v,w)) continue;
            if (dfn[v]==0){
                call_stack[call_head++] = v;
                edge_offset[u]=begin_offset;
                breakdown = true;
                break;
            }else if (label[v]==UINT_N_MAX){
                low[u]=min(low[u],dfn[v]);
            }
        }
        if (breakdown){
            continue;
        }
        if (low[u]==dfn[u]){
            while(1){
                NodeId x = stack[--stack_head];
                label[x]=cnt;
                if (x==u) break;
            }
            cnt++;
        }
        call_head--;
    }
}

size_t Tarjan_SCC::scc(sequence<NodeId>& label){
    parallel_for(0,G.n,[&](size_t i){
        label[i]=UINT_N_MAX;
        dfn[i]=0;
    });
    idx = cnt = 0;
    stack_head = call_head = 0;
    for (size_t i = 0; i<G.n; i++){
        if (dfn[i]==0) dfs((NodeId)i, label);
    }
    return cnt;
}