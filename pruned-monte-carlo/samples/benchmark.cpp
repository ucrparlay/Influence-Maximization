#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <stack>
#include <algorithm>
#include "../src/pmc.hpp"
#include "../../graph.hpp"

using namespace std;

// ./benchmark /data/lwang323/graph/bin/Epinions1_sym.bin 5 100

int main(int argc, char **argv) {
	if (argc < 4) {
		cerr << "./pmc graph k R" << endl;
		exit(1);
	}

  auto graph = read_graph(argv[1]);

	int k = atoi(argv[2]);
	int R = atoi(argv[3]);

	vector<pair<pair<int, int>, double> > es;
	int u, v;
	double p;
  for (unsigned int i = 0; i < graph.n; i++) {
    for (unsigned int j = graph.offset[i]; j < graph.offset[i + 1]; j++) {
      u = i;
      v = graph.E[j];
      p = 0.1;
      es.push_back(make_pair(make_pair(u, v), p));
    }
  }

	InfluenceMaximizer im;
	vector<int> seeds = im.run(es, k, R);
	for (int i = 0; i < k; i++) {
		cout << i << "-th seed =\t" << seeds[i] << endl;
	}

	return 0;
}
