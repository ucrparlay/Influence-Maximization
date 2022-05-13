#include "general_cascade.hpp"

#include "graph.hpp"
#include "parseCommandLine.hpp"

using namespace std;

parlay::sequence<NodeId> ReadSeeds(char* file) {
  ifstream fin(file);
  int k;
  fin >> k;
  parlay::sequence<NodeId> seeds(k);
  for (int i = 0; i < k; i++) fin >> seeds[i];
  return seeds;
}

// ./general_cascade /data/lwang323/graph/bin/HepPh_sym.bin seeds_im.txt -w 0.1
int main(int argc, char* argv[]) {
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " graph_file seeds_file\n";
    abort();
  }
  CommandLine P(argc, argv);
  char* graph_file = argv[1];
  char* seeds_file = argv[2];
  auto graph = read_graph(graph_file);
  float w = P.getOptionDouble("-w", 0.0);
  bool random = P.getOption("-random");
  if (w == 0.0) {
    cout << "WIC" << endl;
    AssignIndegreeWeight(graph);
  } else {
    cout << "UIC w=" << w << endl;
    AssignUniWeight(graph, w);
  }
  cout << "n: " << graph.n << " m: " << graph.m << endl;

  auto seeds = ReadSeeds(seeds_file);
  cout << "seeds: ";
  for (auto seed : seeds) cout << seed << " ";
  cout << endl;

  GeneralCascade gc(&graph);
  auto res = gc.Run(seeds, 1000, random);
  cout << res << endl;

  return 0;
}