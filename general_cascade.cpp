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

// ./general_cascade /data/graphs/bin/HT_2_sym.bin seeds.txt -w 0.1
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
  auto res = gc.Run(seeds, 100);
  cout << res << endl;

  return 0;
}