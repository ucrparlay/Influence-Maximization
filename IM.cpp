#include <iostream>
#include "IM.hpp"
#include "parseCommandLine.hpp"


int main(int argc, char* argv[]){
    CommandLine P(argc, argv);
    if (argc < 1) {
        cerr << "Usage: " << argv[0] << " graph\n";
        abort();
    }
    char* file = argv[1];
    size_t k = P.getOptionInt("-k", 5);
    size_t R = P.getOptionInt("-R", 200);
    float w = P.getOptionDouble("-w", 0.0);
    Graph graph = read_graph(file);
    if (w == 0.0){
        AssignIndegreeWeight(graph);
    }else{
        AssignUniWeight(graph, w);
    }
    cout << "n: " << graph.n << " m: " << graph.m << endl; 
    InfluenceMaximizer IM_solver(graph);
    IM_solver.init_sketches(R);
    cout << "k " << k << " R " << R << endl;
    // IM_solver.seeds(k, R);
    return 0;
}