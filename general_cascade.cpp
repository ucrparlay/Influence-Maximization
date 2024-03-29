#include <sstream>

#include "BFS_cascade.hpp"
#include "graph.hpp"
#include "parseCommandLine.hpp"

using namespace std;
using namespace parlay;
template <typename Out>
void split(const std::string& s, char delim, Out result) {
  std::istringstream iss(s);
  std::string item;
  while (std::getline(iss, item, delim)) {
    *result++ = item;
  }
}

sequence<NodeId> split(const std::string& s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, std::back_inserter(elems));
  sequence<NodeId> seeds;
  for (size_t i = 1; i < elems.size(); i++) {
    seeds.push_back(stoi(elems[i]));
  }
  return seeds;
}

sequence<sequence<NodeId>> ReadSeeds(char* file) {
  ifstream fin(file);
  // int k=200;
  size_t r;
  string pattern = "seeds:";
  string strLine;
  sequence<string> lines;
  while (getline(fin, strLine)) {
    if (pattern == strLine.substr(0, pattern.size())) {
      lines.push_back(strLine);
    }
  }
  r = lines.size();
  sequence<sequence<NodeId>> seeds(r);
  for (size_t i = 0; i < r; i++) {
    seeds[i] = split(lines[i], ' ');
  }
  return seeds;
}

// ./general_cascade /data/lwang323/graph/bin/HepPh_sym.bin seeds_im.txt -w 0.1
int main(int argc, char* argv[]) {
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " graph_file seeds_file\n";
    abort();
  }
  CommandLine P(argc, argv);
  char* graph_file = argv[1];
  char* seeds_file = argv[2];
  auto graph = read_graph(graph_file);
  double w = P.getOptionDouble("-UIC", 0.0);
  double u1 = P.getOptionDouble("-ua", 1.0);
  double u2 = P.getOptionDouble("-ub", 1.0);
  bool WIC = P.getOption("-WIC");
  int seeds_id = P.getOptionInt("-id", 0);
  cout << graph_file << endl;
  printf("w: %f ua: %f ub: %f WIC: %d\n", w, u1, u2, WIC);
  // AssignUniWeight(graph, w);
  // AssignUniformRandomWeight(graph, 0, 0.1);
  // AssignWICWeight(graph);
  if (w!=0)  AssignUniWeight(graph, w);
  else if (u2!= 1) AssignUniformRandomWeight(graph, u1 ,u2);
  else if (WIC) AssignWICWeight(graph);
  else {
    cout << "no weight assginment specified. -w [float] for uni weight, -u [float] for uniform (float, float+0.1) -WIC for WIC_SYM" << endl;
    return 1;
  }

  int num_iter = P.getOptionInt("-i", 20000);
  int k = P.getOptionInt("-k", 100);
  cout << "n: " << graph.n << " m: " << graph.m
       << " iter: " << num_iter << endl;
  BFS bfs_solver(graph);
  GeneralCascade gc(bfs_solver);
  auto seeds = ReadSeeds(seeds_file);
  im::timer evalute_time;
  // for (size_t i = 0; i < seeds.size(); i++) {
  sequence<NodeId> seed_i = seeds[seeds_id];
  evalute_time.start();
  auto res = gc.Run(seed_i.subseq(0, k), num_iter);
  double eval_time = evalute_time.stop();
  cout << "evaluation time: " << eval_time << endl;
  printf("influence: %.5lf\n", res);
  // if (eval_time > 600) {
  //   cout << "!!! per evaluation is more than 10min " << endl;
  //   break;
  // }
  // }
  return 0;
}