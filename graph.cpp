#include "graph.hpp"
#include "parseCommandLine.hpp"
#include <iostream>
#include <fstream>

#include <iostream>

void write_weighted_edgelist(Graph& graph, float w, const char* const OutFileName ){
  ofstream myfile (OutFileName);
    if (myfile.is_open())
    {
      myfile << graph.n << " " << graph.m << endl;
      for (size_t i = 0; i<graph.n; i++){
        NodeId u = i;
        for (size_t j = graph.offset[i]; j < graph.offset[i+1]; j++){
          NodeId v = graph.E[j];
          myfile << u << " " << v << " " << w << endl;
        }
      }
      myfile.close();
    }
    else cout << "Unable to open file";
}


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
  // if (w==0.0){
  //   cout << "WIC" << endl;
  //   AssignIndegreeWeight(graph);
  // }else{
  //   cout << "UIC w=" << w << endl;
  //   AssignUniWeight(graph, w);
  // }
  cout << "n: " << graph.n << " m: " << graph.m << endl;
  write_weighted_edgelist(graph, w, outfile);
  return 0;
}