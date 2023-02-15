#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <stack>
#include <algorithm>
#include "../src/pmc.hpp"
#include "../../get_time.hpp"
#include "../../graph.hpp"
#include "parlay/sequence.h"

using namespace std;

// ./pmc /data/lwang323/graph/bin/Youtube_sym.bin 200 200

int main(int argc, char **argv) {
	if (argc < 5) {
		cerr << "./pmc graph k R w" << endl;
		exit(1);
	}

  cout << "graph: " << argv[1] << endl;
  auto graph = read_graph(argv[1]);

	int k = atoi(argv[2]);
	int R = atoi(argv[3]);
  double w = atof(argv[4]);
  cout << "k = " << k << endl;
  cout << "R = " << R << endl;
  cout << "w = " << w << endl;
  cout << "n = " << graph.n << endl;
  cout << "m = " << graph.m << endl;

	vector<pair<pair<int, int>, double> > es;
	int u, v;
	double p;
  for (unsigned int i = 0; i < graph.n; i++) {
    for (unsigned int j = graph.offset[i]; j < graph.offset[i + 1]; j++) {
      u = i;
      v = graph.E[j];
      p = w;
      es.push_back(make_pair(make_pair(u, v), p));
    }
  }

  im::timer tm;
	InfluenceMaximizer im;
	vector<int> seeds = im.run(es, k, R);
  cout << "total time: " << tm.stop() << endl;

  cout << "seeds: ";
	for (int i = 0; i < k; i++) {
		cout << seeds[i] << " \n"[i == k - 1];
	}

	return 0;
}
