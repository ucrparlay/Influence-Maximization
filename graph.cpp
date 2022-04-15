#include "graph.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
  if (argc < 4) {
    cerr << "Usage: " << argv[0] << " infile outfile n\n";
    abort();
  }
  char* infile = argv[1];
  char* outfile = argv[2];
  NodeId n = atoi(argv[3]);
  read_txt(infile, outfile, n);
  return 0;
}