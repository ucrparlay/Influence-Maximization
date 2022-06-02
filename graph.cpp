#include "graph.hpp"
#include "parseCommandLine.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
  if (argc < 3) {
    cerr << "Usage: " << argv[0] << " file\n";
    abort();
  }
  CommandLine P(argc, argv);
  char* infile = argv[1];
  char* outfile = argv[2];
  auto graph = read_graph(infile);
  float w = P.getOptionDouble("-w", 0.0);
  if (w==0.0){
    cout << "WIC" << endl;
    AssignIndegreeWeight(graph);
  }else{
    cout << "UIC w=" << w << endl;
    AssignUniWeight(graph, w);
  }
  cout << "n: " << graph.n << " m: " << graph.m << endl;
  // WriteSampledGraph(graph, outfile);
  return 0;
}