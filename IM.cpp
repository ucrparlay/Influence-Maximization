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
  double u1 = P.getOptionDouble("-ua", 1.0);
  double u2 = P.getOptionDouble("-ub", 1.0);
  bool WIC = P.getOption("-WIC");
  float compact = P.getOptionDouble("-compact", 1.0);
  printf("w: %f ua: %f ub: %f WIC: %d k: %d R: %d compact: %f\n", w, u1, u2, WIC, k, R, compact);
  Graph graph = read_graph(file);
  if (w!=0)  AssignUniWeight(graph, w);
  else if (u2!= 1) AssignUniformRandomWeight(graph, u1 ,u2);
  else if (WIC) AssignWICWeight(graph);
  else {
    cout << "no weight assginment specified. -w [float] for uni weight, -u [float] for uniform (float, float+0.1) -WIC for WIC_SYM" << endl;
    return 1;
  }

#if defined(MEM)
  cout << "**size of graph is "
       << sizeof(graph) + (sizeof(NodeId) + sizeof(float)) * graph.m +
              sizeof(EdgeId) * graph.n
       << endl;
#endif
  compact = P.getOptionDouble("-compact", 1.0);
  // cout << "compact " << compact << endl;
  // tt.start();
  
  // compact_IM_solver.init_sketches();
  // double sketch_time = tt.stop();
  // cout << "sketch construction time: " << sketch_time << endl;
  // tt.start();

  
  double sketch_time, select_time, total_time=0;
  int t = P.getOptionInt("-t", 3);
  double temp_time;
  timer IM_tt;
  sequence<pair<NodeId, float>> seeds;

  for (int i = 0;i<t+1; i++){
    IM_tt.start();
    CompactInfluenceMaximizer compact_IM_solver(graph, compact, R);
    compact_IM_solver.init_sketches();
    temp_time = IM_tt.stop();
    printf("round %d \n", i);
    printf("    sketch time %f \n", temp_time);
    sketch_time += temp_time;
    auto select_seeds = [&](int k){
      if (P.getOption("-Q")) {
        return compact_IM_solver.select_seeds_prioQ(k);
      } else if (P.getOption("-PAM")) {
        return compact_IM_solver.select_seeds_PAM(k);
      } else {
        return compact_IM_solver.select_seeds(k);
      }
    };
    IM_tt.start();
    seeds = select_seeds(k);
    temp_time = IM_tt.stop();
    printf("    select time %f \n", temp_time);
    select_time += temp_time;
    if (i == 0){
      select_time = 0; sketch_time=0;
      IM_tt.reset();
    }
  }
  total_time = IM_tt.get_total();
  cout << "average sketch construction time: " << sketch_time/t << endl;
  cout << "average seed selection time: " << select_time/t << endl;
  cout << "average total time: " << total_time/t << endl;
  cout << "seeds: ";
  for (auto s : seeds) cout << s.first << ' ';
  cout << endl;
  return 0;
}