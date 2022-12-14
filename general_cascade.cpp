// #include "general_cascade.hpp"
#include "BFS_cascade.hpp"
#include "graph.hpp"
#include "parseCommandLine.hpp"

using namespace std;
using namespace parlay;
template <typename Out>
void split(const std::string &s, char delim, Out result) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        *result++ = item;
    }
}

sequence<NodeId> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    sequence<NodeId> seeds;
    for (size_t i = 1; i< elems.size(); i++){
      seeds.push_back(stoi(elems[i]));
    }
    return seeds;
}

sequence<sequence<NodeId>> ReadSeeds(char* file) {
  ifstream fin(file);
  // int k=200;
  size_t r;
  string pattern="seeds:";
  string strLine;
  sequence<string> lines;
  while(getline(fin, strLine)){
    if (pattern == strLine.substr(0, pattern.size())){
      lines.push_back(strLine);
    }
  }
  r = lines.size();
  sequence<sequence<NodeId>> seeds(r);
  for (size_t i = 0; i<r; i++){
    seeds[i]= split(lines[i], ' ');
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
  // Graph graph = read_txt(graph_file);
  float w = P.getOptionDouble("-w", 0.02);
  AssignUniWeight(graph,w);
  int num_iter = P.getOptionInt("-i", 20000);
  int k = P.getOptionInt("-k", 100);
  // bool random = P.getOption("-random");
  cout << "n: " << graph.n << " m: " << graph.m << endl;
  BFS bfs_solver(graph, w);
  GeneralCascade gc(bfs_solver);
  auto seeds = ReadSeeds(seeds_file);
  // sequence<int> K = {50,100,150,200};
  timer evalute_time;
  // for (auto k: K){
  // int k = 100;
  // int k = 53;
  for (size_t i = 0; i<seeds.size(); i++){
    sequence<NodeId> seed_i = seeds[i];
    cout << " k is " << k << endl;
    evalute_time.start();
    auto res = gc.Run(seed_i.subseq(0,k), num_iter);
    cout << "time: " << evalute_time.stop() << endl;
    cout << res << endl;
  }
  // }

  return 0;
}