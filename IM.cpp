#include <iostream>

#include "IM_compact.hpp"
#include "get_time.hpp"
#include "parseCommandLine.hpp"

int main(int argc, char* argv[]) {
  CommandLine P(argc, argv);
  if (argc < 1) {
    cerr << "Usage: " << argv[0] << " graph\n";
    abort();
  }
  char* file = argv[1];
  size_t k = P.getOptionInt("-k", 100);
  size_t R = P.getOptionInt("-R", 256);
  float w = P.getOptionDouble("-w", 0.02);
  float compact = P.getOptionDouble("-compact", 1.0);
  Graph graph = read_graph(file);
  AssignUniWeight(graph, w);
  cout << "n: " << graph.n << " m: " << graph.m << " w: " << w << " R: " << R
       << " k: " << k << endl;
#if defined(MEM)
  cout << "**size of graph is "
       << sizeof(graph) + (sizeof(NodeId) + sizeof(float)) * graph.m +
              sizeof(EdgeId) * graph.n
       << endl;
#endif
  if (P.getOption("-compact")) {
    timer tt;
    cout << "compact " << compact << endl;
    tt.start();
    CompactInfluenceMaximizer compact_IM_solver(graph, compact, R);
    compact_IM_solver.init_sketches();
    double sketch_time = tt.stop();
    cout << "sketch construction time: " << sketch_time << endl;
    tt.start();
    sequence<pair<NodeId, float>> seeds;
    if (P.getOption("-Q")) {
      seeds = compact_IM_solver.select_seeds_prioQ(k);
    } else if (P.getOption("-PAM")) {
      seeds = compact_IM_solver.select_seeds_PAM(k);
    } else {
      seeds = compact_IM_solver.select_seeds(k);
    }
    double select_time = tt.stop();
    cout << "seed selection time: " << select_time << endl;
    cout << "total time: " << select_time + sketch_time << endl;
    cout << "seeds: ";
    for (auto s : seeds) cout << s.first << ' ';
    cout << endl;
  }
  return 0;
}