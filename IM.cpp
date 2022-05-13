#include <iostream>
#include "IM.hpp"
#include "parseCommandLine.hpp"
#include "get_time.hpp"

// ./IM /data/graphs/bin/com-orkut_sym.bin -k 200 -R 200 -w 0.1
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
    Graph graph = read_graph(file);
    if (w == 0.0){
        cout << "WIC" << endl;
        AssignIndegreeWeight(graph);
    }else{
        cout << "UIC w: " << w<<  endl;
        AssignUniWeight(graph, w);
    }
    cout << "n: " << graph.n << " m: " << graph.m << endl;
    InfluenceMaximizer IM_solver(graph);
    // timer t;
    // float cost;
    // IM_solver.init_sketches(R, true);
    // cost = t.stop();
    // cout << "parallel{parallel} init_sketches: " << cost << endl;
    // t.start();
    // IM_solver.init_sketches(R, false);
    // cost = t.stop();
    // cout << "sequential{parallel} init_sketches: " << cost << endl;

    timer t;
    float cost;
    auto seeds_spread = IM_solver.select_seeds(k, R);
    cost = t.stop();
    cout << "total time: " << cost << " spread " << seeds_spread.second << endl;
    for (auto x: seeds_spread.first) {
      cout << x << " ";
    }
    cout << endl;
    
    return 0;
}