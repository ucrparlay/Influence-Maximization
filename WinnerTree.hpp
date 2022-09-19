#pragma once
#include "get_time.hpp"
#include "parlay/sequence.h"
#include "utilities.h"
using namespace std;
using namespace parlay;

// tree stores <influence, value's index(or saying the vertex id)>
void build_up(sequence<size_t>& value, 
            sequence<pair<size_t, NodeId>>& tree,
            NodeId start, NodeId end){
    if (start == end - 1){
        return;
    }
    NodeId m = (start+end) >>1;
    par_do(
        [&](){build_up(value, tree, start, m);},
        [&](){build_up(value, tree, m, end);});
    NodeId l_idx = (start+m)>>1;
    NodeId r_idx = (m+end)>>1;
    // pair<size_t, NodeId> left, right;
    auto left = (l_idx == start)? make_pair(value[start], start): tree[l_idx];
    auto right = (r_idx == m)? make_pair(value[m],m): tree[r_idx];
    pair<size_t, NodeId> winner = (left.first >right.first )? left :right;
    tree[m]= winner;
    return;
}

void update(sequence<size_t>& value, 
            sequence<pair<size_t, NodeId>>& tree, 
            NodeId start, NodeId end){
    if (start +1 == end){
        return;
    }
    if (start +2 == end){
        auto winner = (value[start] > value[start+1])?
            make_pair(value[start], start): make_pair(value[start+1],start+1);
        tree[start+1] = winner;
        return;
    }
    NodeId m = (start+end)>>1;
    auto root = tree[m].second;
    NodeId l_idx = (start+m)>>1;
    NodeId r_idx = (m+end)>>1;
    pair<size_t, NodeId> left, right;
    if (l_idx != start && tree[l_idx].second == root){
        update(value, tree, start, m);
    }else if (r_idx != m && tree[r_idx].second == root){
        update(value, tree, m, end);
    }
    // else don't need to do anything
    
    left = (l_idx == start)? make_pair(value[start], start): tree[l_idx];
    right = (r_idx == m)? make_pair(value[m],m): tree[r_idx];
    pair<size_t, NodeId> winner = (left.first > right.first)?
                                        left : right;
    tree[m]=winner;
    return;
}