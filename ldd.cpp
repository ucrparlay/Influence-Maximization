#include "ldd.hpp"

#include "get_time.hpp"
#include "graph.hpp"
#include "parseCommandLine.hpp"
using namespace std;

int main(int argc, char** argv) {
  CommandLine P(argc, argv);
  if (argc < 2) {
    cout << "usage: ./ldd fileName" << endl;
    return 0;
  }
  char* filename = argv[1];
  timer t1;
  t1.start();
  Graph G = read_graph(filename);
  t1.stop();

  cout << "#vertices: " << G.n << " #edges: " << G.m << '\n';

  LDD LDD_P(G);

  int repeat = P.getOptionInt("-t", (int)3);
  double beta = P.getOptionDouble("-beta", (double)0.2);
  LDD_P.ldd(beta);

  timer t;
  for (int i = 0; i < repeat; i++) {
    t.start();
    LDD_P.ldd(beta);
    double cost = t.stop();
    cout << "ldd cost: " << cost << " round: " << LDD_P.num_round << " "
         << endl;
  }
  cout << "average cost: " << t.get_total() / repeat << endl;
  return 0;
}
