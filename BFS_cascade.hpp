#pragma once

#include <random>

#include "graph.hpp"
#include "bfs.hpp"
#include "parlay/sequence.h"
#include "utilities.h"

using namespace std;

struct GeneralCascade {
  private:
    BFS BFS_simulate;
  public:
  GeneralCascade(BFS _bfs) : BFS_simulate(_bfs) {}

  double Run(const parlay::sequence<NodeId>& seeds, int num_iter) {
    cout << seeds.size() <<" seeds: ";
    for (auto s: seeds){
      cout << s << " ";
    }
    cout << endl;
    size_t influence = 0;
    BFS_simulate.rand_seed = 0;
    for (int i = 0; i<num_iter; i++){
      Hash_Edge hash_edge = {_hash((EdgeId) i)};
      size_t single_spread = BFS_simulate.bfs(seeds, hash_edge);
      // size_t check_spread = BFS_simulate.bfs_sequence(seeds, hash_edge);
      // if (single_spread != check_spread){
      //   cout << "check failed " << single_spread << " " << check_spread << endl;
      // }
      influence+= single_spread;
    }
    return influence / (num_iter+0.0);
  }
};
