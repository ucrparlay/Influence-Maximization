#include "reach.hpp"

#include <algorithm>  // std::max
#include <iostream>

#include "get_time.hpp"
#include "graph.hpp"
#include "parseCommandLine.hpp"
#include "utilities.h"

using namespace std;

int main(int argc, char** argv) {
  CommandLine P(argc, argv);
  if (argc < 2) {
    cout << "usage: ./reach fileName" << endl;
    return 0;
  }
  char* fileName = argv[1];
  timer t1;
  t1.start();
  cout << "symmetric " << P.getOptionInt("-s", 0) << endl;
  Graph graph = read_graph(
      fileName, P.getOptionInt("-s", 0));  // symmetric : 1, directed; 2

  t1.stop();
  cout << "read graph finish" << endl;
  cout << "n: " << graph.n << " m: " << graph.m << endl;

  REACH REACH_P(graph);

  sequence<bool> dst(graph.n);

  NodeId source = P.getOptionInt("-r", (NodeId)0);
  int repeat = P.getOptionInt("-t", (int)3);
  bool local = P.getOption("-local");
  timer t;
  REACH_P.reach(source, dst, local);
  double reach_cost;
  for (int i = 0; i < repeat; i++) {
    t.start();
    REACH_P.reach(source, dst, local);
    reach_cost = t.stop();
    cout << "reachability cost: " << reach_cost << " round "
         << REACH_P.num_round << endl;
  }
  cout << "average cost " << t.get_total() / repeat << endl;

  if (P.getOption("-c")) {  // check

    bool success = true;
    sequence<bool> dst_seq(graph.n);
    timer t3;
    t3.start();
    REACH_P.reach_seq(source, dst_seq);
    t3.stop();
    cout << "sequential bfs cost " << t3.get_total() << endl;
    cout << "checking answer" << endl;
    for (size_t i = 0; i < graph.n; i++) {
      if (dst_seq[i] != dst[i]) {
        cout << "wrong i = " << i << endl;
        cout << "dst_seq " << dst_seq[i] << endl;
        cout << "dst " << dst[i] << endl;
        cout << "WRONG!" << endl;
        success = false;
        break;
      }
    }
    if (success)
      cout << "PASS! "
           << "Cost: " << t3.get_total() << endl;
  }
  // if (P.getOption("-component")){
  //     auto visited = sequence::sum(dst, graph.n);
  //     cout << "reached " << visited << " nodes" << endl;
  //     cout << "unreached " << graph.n-visited << " nodes" << endl;
  // }

  return 0;
}
