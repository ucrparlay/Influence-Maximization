#include <iostream>

#include "IM_compact.hpp"
#include "get_time.hpp"
#include "parseCommandLine.hpp"




int main(int argc, char* argv[]) {
  cout << "num_workers: " << parlay::num_workers() << endl;
  CommandLine P(argc, argv);
  if (argc < 1) {
    cerr << "Usage: " << argv[0] << " graph\n";
    abort();
  }
  for (int i = 0; i<argc; i++){
    cout << argv[i] << " ";
  }
  cout << endl;
  char* file = argv[1];
  size_t k = P.getOptionInt("-k", 100);
  size_t R = P.getOptionInt("-R", 256);
  double w = P.getOptionDouble("-UIC", 0.0);
  double u = P.getOptionDouble("-UNIF", -1.0);
  bool WIC = P.getOption("-WIC");
  float compact = P.getOptionDouble("-compact", 1.0);
  printf("w: %f u: %f WIC: %d k: %d R: %d compact: %f\n", w, u, WIC, k, R, compact);
  Graph graph = read_graph(file);
  if (w!=0)  AssignUniWeight(graph, w);
  else if (u!= -1) AssignUniformRandomWeight(graph, u ,u+0.1);
  else if (WIC) AssignWICWeight(graph);
  else {
    cout << "no weight assginment specified. -w [float] for uni weight, -u [float] for uniform (float, float+0.1) -WIC for WIC_SYM" << endl;
    return 1;
  }
  // 
  cout << "n: " << graph.n << " m: " << graph.m;
  if (w!=0) cout << " weight " << w;
  else if (u != -1) cout << " uniform(" << u << "," << u+0.1<<")";
  else if (WIC) cout << "WIC_SYM";
  
  cout << " R: " << R
       << " k: " << k << endl;
#if defined(MEM)
  cout << "**size of graph is "
       << sizeof(graph) + (sizeof(NodeId) + sizeof(float)) * graph.m +
              sizeof(EdgeId) * graph.n
       << endl;
#endif
  compact = P.getOptionDouble("-compact", 1.0);
  cout << "compact " << compact << endl;
  // tt.start();
  CompactInfluenceMaximizer compact_IM_solver(graph, compact, R);
  // compact_IM_solver.init_sketches();
  // double sketch_time = tt.stop();
  // cout << "sketch construction time: " << sketch_time << endl;
  // tt.start();

  
  auto select_seeds = [&](int k){
    if (P.getOption("-Q")) {
      return compact_IM_solver.select_seeds_prioQ(k);
    } else if (P.getOption("-PAM")) {
      return compact_IM_solver.select_seeds_PAM(k);
    } else {
      return compact_IM_solver.select_seeds(k);
    }
  };
  
  double sketch_time, select_time, total_time=0;
  int t = P.getOptionInt("-t", 3);
  double temp_time;
  timer IM_tt;
  sequence<pair<NodeId, float>> seeds;

  for (int i = 0;i<t; i++){
    IM_tt.start();
    compact_IM_solver.init_sketches();
    temp_time = IM_tt.stop();
    printf("round %d: sketch time %f \n", i+1, temp_time);
    sketch_time += temp_time;
    IM_tt.start();
    seeds = select_seeds(k);
    temp_time = IM_tt.stop();
    printf("round %d: select time %f \n", i+1, temp_time);
    select_time += temp_time;
  }
  total_time = IM_tt.get_total();
  cout << "sketch construction time: " << sketch_time/t << endl;
  cout << "seed selection time: " << select_time/t << endl;
  cout << "total time: " << total_time/t << endl;
  cout << "seeds: ";
  for (auto s : seeds) cout << s.first << ' ';
  cout << endl;
  return 0;
}