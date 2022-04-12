#include "connect.h"

#include <parlay/monoid.h>
#include <parlay/primitives.h>

#include "parseCommandLine.hpp"

using namespace std;
using namespace parlay;

int main(int argc, char** argv) {
  CommandLine P(argc, argv);
  if (argc < 2) {
    cout << "usage: ./connect filename" << endl;
    return 0;
  }
  char* filename = argv[1];
  timer t1;
  t1.start();
  Graph G = read_graph(filename, !P.getOption("-nmmap"));
  t1.stop();
  NodeId graph_id = P.getOptionInt("-i",0);
  float w = P.getOptionDouble("-w", 1.0);
  Hash_Edge hash_edge{graph_id, false, w};

  cout << "#vertices: " << G.n << " #edges: " << G.m << '\n';

  int repeat = P.getOptionInt("-t", (int)10);
  double beta = P.getOptionDouble("-beta", (double)0.2);
  timer t;
  connect(G, beta, hash_edge);
  // ofstream ofs("connectivity.dat", ios_base::app);
  for (int i = 0; i < repeat; i++) {
    t.start();
    connect(G, beta, hash_edge);
    double time = t.stop();
    // ofs << time << '\n';
    cout << "connect_cost: " << time << " " << endl;
  }
  cout << "average_time: " << t.get_total() / repeat << " " << endl;
  // ofs.close();
  return 0;
}
