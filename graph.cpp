#include "graph.hpp"
#include "parseCommandLine.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " file\n";
    abort();
  }
  CommandLine P(argc, argv);
  char* infile = argv[1];
  auto graph = read_graph(infile);
  AssignIndegreeWeight(graph);
  return 0;
}