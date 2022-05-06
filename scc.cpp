#include "scc.hpp"

#include <algorithm>  // std::max
#include <vector>
#include <unordered_set>

#include "get_time.hpp"
#include "parseCommandLine.hpp"

using namespace std;


void SCC_status(sequence<size_t> label, size_t n) {
  sequence<NodeId> flag(n + 1);
  NodeId MASK = UINT_N_MAX >> 1;
  parallel_for(0, n + 1, [&](NodeId i) { flag[i] = 0; });
  for (size_t i = 0; i < n; i++) {
    flag[label[i] & MASK]++;
  }
  // cout << "n_scc = " << sequence::sum(flag, n, [&](NodeId label_size){return
  // label_size!=0;}) << endl;
  cout << "n_scc = " << parlay::count_if(flag, [&](NodeId label_size) {
    return label_size != 0;
  }) << endl;
  cout << "Largest StronglyConnectedComponents has "
       << parlay::reduce(flag, maxm<NodeId>()) << endl;
}

struct hashFunction{
  size_t operator()(const pair<NodeId, NodeId>& edge) const{
    return _hash(_hash(edge.first)+edge.second);
  }
};

void SCC_statistic(Graph graph, sequence<size_t> label, Hash_Edge& hash_edge){
  size_t n = graph.n;
  sequence<NodeId> scc_sizes(n);
  parallel_for(0, n, [&](size_t i){scc_sizes[i]=0;});
  for (size_t i = 0; i<n; i++){
    scc_sizes[label[i]] ++;
  }
  unordered_set<pair<NodeId,NodeId>, hashFunction> edge_set;
  for (size_t i = 0; i<n; i++){
    NodeId u = i;
    for (size_t j = graph.offset[i]; j<graph.offset[i+1]; j++){
      NodeId v = graph.E[j];
      float w = graph.W[j];
      if (hash_edge(u,v,w) && label[u]!= label[v]){
        edge_set.insert(make_pair(label[u], label[v]));
      }
    }
  }
  cout << "n_scc = " << parlay::count_if(scc_sizes, [&](NodeId scc_size) {
    return scc_size != 0;
  }) << endl;
  cout << "Largest StronglyConnectedComponents has "
       << parlay::reduce(scc_sizes, maxm<NodeId>()) << endl;
  cout << "number of inter_scc edges = " << edge_set.size() << endl;
  parlay::sort_inplace(scc_sizes);
  cout << "Largest 200 SCC size:" << endl;
  for (size_t i = 0; i<200; i++){
    cout << scc_sizes[n-i-1] << endl;
  }
}

int main(int argc, char** argv) {
  CommandLine P(argc, argv);
  if (argc < 2) {
    cout << "usage: ./scc fileName" << endl;
    return 0;
  }
  char* fileName = argv[1];
  timer t1;
  t1.start();
  double beta = P.getOptionDouble("-beta", 1.5);
  cout << "--beta " << beta << "--" << endl;

  Graph graph = read_graph(
      fileName, !P.getOption("-nmmap"));  // symmetric : 1, makeSymmetric; 2,
                                          // makeDirected // binary
  // Graph graph = read_large_graph(fileName);
  bool local_reach = P.getOption("-local_reach");
  bool local_scc = P.getOption("-local_scc");

  t1.stop();
  cout << "read graph finish" << endl;
  cout << "n: " << graph.n << " m: " << graph.m << endl;

  // size_t* label = new size_t[graph.n];
  sequence<size_t> label = sequence<size_t>(graph.n);
  int repeat = P.getOptionInt("-t", (int)5);
  NodeId graph_id = P.getOptionInt("-i", 0);
  float w = P.getOptionDouble("-w", 0.0);
  if (w == 0.0){
    cout << "WIC" << endl;
    AssignIndegreeWeight(graph);
  }else{  
    cout << "UIC" << endl;
    AssignUniWeight(graph,w);
  }
  Hash_Edge hash_edge{graph_id, true};
  timer t;
  SCC SCC_P(graph);
  SCC_P.front_thresh = P.getOptionInt("-thresh", 1000000000);
  SCC_P.scc(label, hash_edge, beta, local_reach, local_scc);
  SCC_P.timer_reset();
  double scc_cost;
  for (int i = 0; i < repeat; i++) {
    t.start();
    SCC_P.scc(label, hash_edge, beta, local_reach, local_scc);
    scc_cost = t.stop();
    cout << "scc cost: " << scc_cost << endl;
  }
  cout << "average cost " << t.get_total() / repeat << endl;
#if defined(BREAKDOWN)
  SCC_P.breakdown_report(repeat);
#endif
  if (P.getOption("-status")) SCC_status(label, graph.n);
  if (P.getOption("-print")) output_component_sizes(label, graph.n);
  if (P.getOption("-static")) {
    hash_edge.forward = true;
    SCC_statistic(graph,label, hash_edge);
  }

  return 0;
}