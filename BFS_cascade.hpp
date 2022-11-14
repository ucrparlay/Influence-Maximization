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
    size_t influence = 0;
    for (int i = 0; i<num_iter; i++){
        influence+=BFS_simulate.bfs(seeds);
    }
    return influence / (num_iter+0.0);
  }
};
