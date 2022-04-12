#include "bfs.hpp"

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
    cout << "usage: ./bfs fileName" << endl;
    return 0;
  }
  char* fileName = argv[1];
  timer t1;
  t1.start();
  Graph graph =
      read_graph(fileName);  // symmetric : 1, makeSymmetric; 2, makeDirected

  t1.stop();
  cout << "read graph finish" << endl;
  cout << "n: " << graph.n << " m: " << graph.m << endl;

  BFS BFS_P(graph);

  sequence<NodeId> dst(graph.n);

  NodeId source = P.getOptionInt("-r", (NodeId)0);
  // cout << "source " << source << " out degree " << graph.offset[source+1] -
  // graph.offset[source]<< " in degree "<< graph.in_offset[source+1] -
  // graph.in_offset[source] << endl;
  int repeat = P.getOptionInt("-t", (int)3);
  // bool mute = P.getOption("-um");  // unmute
  timer t;

  BFS_P.bfs(source, dst);
  double bfs_cost;
  for (int i = 0; i < repeat; i++) {
    t.start();
    BFS_P.bfs(source, dst);
    bfs_cost = t.stop();
    cout << "bfs cost: " << bfs_cost << " round " << BFS_P.num_round << endl;
  }
  cout << "average cost " << t.get_total() / repeat << endl;

  if (P.getOption("-c")) {  // check

    bool success = true;
    sequence<NodeId> dst_seq(graph.n);
    // uintT* dst_seq = new uintT[graph.n];
    timer t3;
    t3.start();
    BFS_P.bfs_seq(source, dst_seq);
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

  return 0;
}
