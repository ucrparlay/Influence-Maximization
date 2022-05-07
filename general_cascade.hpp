#pragma once

#include <random>

#include "graph.hpp"
#include "parlay/sequence.h"
#include "utilities.h"

using namespace std;

struct GeneralCascade {
  GeneralCascade(Graph* graph) : graph(graph) {}

  double Run(const parlay::sequence<NodeId>& seeds, int num_iter) {
    auto Simulate = [&]() -> double {
      parlay::sequence<bool> activated(graph->n, false);
      int num = 0;
      auto frontier = seeds;
      while (!frontier.empty()) {
        parlay::parallel_for(0, frontier.size(), [&](int i) {
          assert(!activated[frontier[i]]);
          activated[frontier[i]] = true;
        });
        num += frontier.size();
        parlay::sequence<parlay::sequence<NodeId>> new_nodes(frontier.size());
        parlay::parallel_for(0, frontier.size(), [&](int i) {
          auto u = frontier[i];
          auto nghs = parlay::delayed_tabulate(
              graph->offset[u + 1] - graph->offset[u], [&](int j) {
                auto offset = graph->offset[u] + j;
                return make_pair(graph->E[offset], graph->W[offset]);
              });
          auto good_nghs = parlay::filter(nghs, [&](pair<NodeId, float> p) {
            auto v = p.first;
            auto w = p.second;
            return !activated[v] && (double)rand() / (double)RAND_MAX < w;
          });
          new_nodes[i] = parlay::map(
              good_nghs, [](pair<NodeId, float> p) { return p.first; });
        });
        auto all = parlay::flatten(new_nodes);
        parlay::sort_inplace(all);
        auto new_frontier = parlay::unique(all);
        frontier = new_frontier;
      }
      return num;
    };
    parlay::sequence<double> res(num_iter);
    parlay::parallel_for(0, num_iter, [&](int i) { res[i] = Simulate(); });
    auto tot = parlay::reduce(res);
    return tot / num_iter;
  }

  Graph* graph;
};
