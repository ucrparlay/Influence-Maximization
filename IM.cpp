#include <iostream>
#include "IM.hpp"
#include "IM_compact.hpp"
#include "parseCommandLine.hpp"
#include "get_time.hpp"

// ./IM /data/lwang323/graph/bin/Youtube_sym.bin -k 200 -R 200 -w 0.1 -celf
int main(int argc, char* argv[]){
    CommandLine P(argc, argv);
    if (argc < 1) {
        cerr << "Usage: " << argv[0] << " graph\n";
        abort();
    }
    char* file = argv[1];
    size_t k = P.getOptionInt("-k", 200);
    size_t R = P.getOptionInt("-R", 200);
    float w = P.getOptionDouble("-w", 0.0);
    int option = P.getOptionInt("-option", 2);
    bool CELF = P.getOption("-celf");
    float compact = P.getOptionDouble("-compact", 0.0);
    Graph graph = read_graph(file);
    if (w == 0.0){
        cout << "WIC" << endl;
        AssignIndegreeWeight(graph);
    }else{
        cout << "UIC w: " << w<<  endl;
        AssignUniWeight(graph, w);
    }
    cout << "n: " << graph.n << " m: " << graph.m << endl;
    if (P.getOption("-compact")) {
      timer tt;
      CompactInfluenceMaximizer compact_IM_solver(graph, compact, R);
      compact_IM_solver.init_sketches();
      auto seeds = compact_IM_solver.select_seeds(200);
      cout << "total time: " << tt.stop() << endl;
      cout << "seeds: ";
      for (auto t: seeds) cout << t.first << ' ';
      cout << endl;
      return 0;
    }
    
    InfluenceMaximizer IM_solver(graph);
    timer t_init;
    float cost;
    t_init.start();
    IM_solver.init_sketches(R, option);
    cost = t_init.stop();
    cout << "parallel{edges} init_sketches: " << cost<< endl;
    
    timer t_select;
    auto seeds_spread = IM_solver.select_seeds(k, R, CELF);
    cost = t_select.stop();
    cout << "select time: " << cost  << endl;
    for (auto x: seeds_spread) {
      cout << x.first << " ";
    }
    cout << endl;
    return 0;
}