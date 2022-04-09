#include <iostream>
#include "IM.hpp"

int main(int argc, char* argv[]){
    if (argc < 4) {
        cerr << "Usage: " << argv[0] << " graph k R\n";
        abort();
    }
    char* file = argv[1];
    size_t k = atoi(argv[2]);
    size_t R = atoi(argv[3]);
    Graph graph = read_graph(file);
    cout << "n: " << graph.n << " m: " << graph.m << endl; 
    InfluenceMaximizer IM_solver(graph);
    IM_solver.seeds(k, R);
    return 0;
}