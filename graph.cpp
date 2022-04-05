#include "graph.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
  if (argc < 3) {
    cerr << "Usage: " << argv[0] << " infile outfile\n";
    abort();
  }
  char* infile = argv[1];
  char* outfile = argv[2];
  Graph g = read_graph(infile);
  printf("%zu %zu\n", g.n, g.m);
  ofstream ofs(outfile);
  if (ofs.is_open()){
      ofs << g.n << " " << g.m << endl;
      for (size_t i = 0; i<g.n; i++){
          NodeId u = i;
          for (size_t j = g.offset[i]; j<g.offset[i+1]; j++){
              ofs << u << " " << g.E[j] << " " << 0.1 << endl;
          }
      }
      ofs.close();
  }
  return 0;
}