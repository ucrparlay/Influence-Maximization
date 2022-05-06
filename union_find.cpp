#include <iostream>
#include "get_time.hpp"
#include "union_find.hpp"
#include "parseCommandLine.hpp"
int main(int argc, char* argv[]){
    CommandLine P(argc, argv);
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " graph\n";
        abort();
    }
    char* file = argv[1];
    float w = P.getOptionDouble("-w", 0.0);
    Graph graph = read_graph(file);
    if (w == 0.0){
        AssignIndegreeWeight(graph);
    }else{
        AssignUniWeight(graph, w);
    }
    cout << "n: " << graph.n << " m: " << graph.m << endl; 
    Hash_Edge hash_edge;
    hash_edge.graph_id = 0;
    hash_edge.forward = true;
    timer t;
    auto label = union_find(graph,hash_edge);
    for ( size_t r = 0; r<5; r++){
        t.start();
        label = union_find(graph, hash_edge);
        double time = t.stop();
        cout << "cost time: " << time << endl;
    }
    cout << t.get_total() / 5.0 << endl;
    size_t sum=0;
    bool check_success=true;

    for(size_t i = 0; i< graph.n; i++){
        size_t p_i = label[i];
        if (p_i & TOP_BIT){
            sum += (VAL_MASK& p_i);
        }else{
            if ( (label[p_i]&TOP_BIT) == 0){
                check_success = false;
            }
        }
    }
    if (check_success==false){
        cout << "check fail, point to not root parent" << endl;
    }
    if (sum != graph.n){
        cout << "check fail, sum of components is " << sum << " not equal to " << graph.n << endl;
    }
    cout << "check done" << endl;
    return 0;
}