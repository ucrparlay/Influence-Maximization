#include <algorithm>  // std::max
#include <unordered_set>

#include "get_time.hpp"
#include "parseCommandLine.hpp"
#include "Tarjan_scc.hpp"


using namespace std;



int main(int argc, char** argv) {
  CommandLine P(argc, argv);
  if (argc < 2) {
    cout << "usage: ./tarjan_scc fileName" << endl;
    return 0;
  }
  char* fileName = argv[1];
  Graph graph = read_graph(fileName);
  cout << fileName << " n " << graph.n << " m " << graph.m << endl;

  float w = P.getOptionDouble("-w", 0.0);
  if (w == 0.0){
    cout << "WIC" << endl;
    AssignIndegreeWeight(graph);
  }else{  
    cout << "UIC" << endl;
    AssignUniWeight(graph,w);
  }

  int repeat = P.getOptionInt("-t", (int)3);
  NodeId graph_id = P.getOptionInt("-i", 0);
  timer t;
  Hash_Edge hash_edge{graph_id, (NodeId)graph.n, true};
  sequence<NodeId> label(graph.n);
  Tarjan_SCC SCC_P(graph, hash_edge);
  SCC_P.scc(label);
  double scc_cost;
  size_t n_scc;
  for (int i = 0; i < repeat; i++) {
    t.start();
    n_scc = SCC_P.scc(label);
    scc_cost = t.stop();
    cout << scc_cost << endl;
  }
  cout << "average cost " << t.get_total()/repeat << endl;
  cout << "n_scc " << n_scc << endl;

  return 0;
}
