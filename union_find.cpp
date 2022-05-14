#include <iostream>
#include "get_time.hpp"
#include "union_find.hpp"
#include "parseCommandLine.hpp"
bool check(sequence<size_t> label, size_t n){
    size_t sum=0;
    bool check_success=true;

    for(size_t i = 0; i< n; i++){
        size_t p_i = label[i];
        if (p_i & TOP_BIT){
            sum += (VAL_MASK & p_i);
        }else{
            if ( (label[p_i]&TOP_BIT) == 0){
                check_success = false;
            }
        }
    }
    if (check_success==false){
        cout << "check fail, point to not root parent" << endl;
        return false;
    }
    if (sum != n){
        cout << "check fail, sum of components is " << sum << " not equal to " << n << endl;
        return false;
    }
    return true;
}

int main(int argc, char* argv[]){
    CommandLine P(argc, argv);
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " graph\n";
        abort();
    }
    char* file = argv[1];
    float w = P.getOptionDouble("-w", 0.0);
    size_t r = P.getOptionInt("-r", 20);
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
    sequence<size_t> label = union_find_serial(graph,hash_edge);
    for ( size_t i = 0; i<r; i++){
        hash_edge.graph_id = i;
        hash_edge.forward = true;
        t.start();
        label = union_find_serial(graph, hash_edge);
        double time = t.stop();
        cout << "cost time: " << time << endl;
    }
    cout << "average cost " <<t.get_total() / (r+0.0) << endl;

    if (P.getOption("-c")){
        if (check(label, graph.n)){
            cout << "check success" << endl;
        }
    }
    return 0;
}