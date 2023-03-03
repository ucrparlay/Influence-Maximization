#include <iostream>
// #include "IM.hpp"
#include "IM_compact.hpp"
#include "parseCommandLine.hpp"
// #include "PMC.hpp"
#include "get_time.hpp"

// ./IM /data/lwang323/graph/bin/Youtube_sym.bin -k 200 -R 200 -w 0.1 -celf
int main(int argc, char* argv[]){
    CommandLine P(argc, argv);
    if (argc < 1) {
        cerr << "Usage: " << argv[0] << " graph\n";
        abort();
    }
    char* file = argv[1];
    size_t k = P.getOptionInt("-k", 100);
    size_t R = P.getOptionInt("-R", 256);
    float w = P.getOptionDouble("-w", 0.02);
    // int option = P.getOptionInt("-option", 2);
    // bool CELF = P.getOption("-celf");
    float compact = P.getOptionDouble("-compact", 1.0);
    // thresh = P.getOptionInt("-thresh", 5);
    // Graph graph = read_txt(file);
    // graph.symmetric = true;
    Graph graph = read_graph(file);
    AssignUniWeight(graph,w);
    cout << "n: " << graph.n << " m: " << graph.m << " w: " << w << " R: " << R << " k: " << k<< endl;
    #if defined(MEM)
    cout << "**size of graph is " << sizeof(graph) + (sizeof(NodeId)+sizeof(float))*graph.m \
                              + sizeof(EdgeId)*graph.n << endl;
    #endif
    if (P.getOption("-compact")) {
      timer tt;
      tt.start();
      cout << "compact " << compact << endl;
      CompactInfluenceMaximizer compact_IM_solver(graph, compact, R);
      compact_IM_solver.init_sketches();
      cout << "sketch construction time: " << tt.stop() << endl;
      tt.start();
      sequence<pair<NodeId, float>> seeds;
      if (P.getOption("-Q")){
        seeds = compact_IM_solver.select_seeds_prioQ(k);
      }else if (P.getOption("-PAM")){
        seeds = compact_IM_solver.select_seeds_PAM(k);
      }else{
        seeds = compact_IM_solver.select_seeds(k);
      }
      cout << "seed selection time: " << tt.stop() << endl;
      cout << "total time: " << tt.get_total() << endl;
      cout << "seeds: ";
      for (auto t: seeds) cout << t.first << ' ';
      cout << endl;
    }
    // else{
    //   InfluenceMaximizer IM_solver(graph, R);
    //   timer t;
    //   float cost;
    //   t.start();
    //   IM_solver.init_sketches(R);
    //   cost = t.stop();
    //   cout << "sketching time: " << cost<< endl;
      
    //   t.start();
    //   auto seeds_spread = IM_solver.select_seeds(k);
    //   cost = t.stop();
    //   cout << "select time: " << cost  << endl;
    //   cout << "total time: " << t.get_total() << endl;
    //   cout << "seeds: ";
    //   for (auto x: seeds_spread) cout << x.first << " ";
    //   cout << endl;
    // }
    return 0;
}