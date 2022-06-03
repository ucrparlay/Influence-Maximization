#include "limit.h"
#include "graph.h"
#include "outside_graph.hpp"
#include "staticgreedy.h"
#include "general_cascade.h"
#include "staticgreedy_directed_new.h"
#include "staticgreedy_basic.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include <math.h>
#include <stdlib.h>
#include <ios>
#include <unistd.h>
#include <string.h>
#include "../get_time.hpp"

using namespace std;

template<typename Run>
double toSimulateOnce(int *seeds, int set_size, Run run)
{
  vector<int> set(MAX_NODE);
  int t;

  for (t = 0; t < set_size; t++)
  {
    set[t] = seeds[t];
  }

  return run(100, t, set);
}


int main(int argc, char * argv[])
{
  time_t t;
  srand((unsigned) time(&t));

  //input parameters
  if (argc < 4) {
    printf("Incomplete parameters, four papameters is needed, including:\n");
    printf("argv[1]: full file_path\n");
    printf("argv[2]: seed set size\n");
    printf("argv[3]: R for staticgreedy\n");
    printf("argv[4]: kind of algorithm, bsg for basic staticgreedy, sgd for staticgreedy with dynamic update on directed graph\n");
    printf("example: ./main hep.txt 50 100 bsg\n");
    return 0;
  }

  char file_path[50] = "";
  strcat(file_path, argv[1]);
  int set_size = atoi(argv[2]);
  int R = atoi(argv[3]);
  string algo = argv[4];

  //build graph
  cout << "graph: " << file_path << endl;
  auto my_graph = outside_graph::TTT::read_graph(file_path);
  cout << "my_graph: " << my_graph.n << " " << my_graph.m << endl;
  Graph::BuildFromFile2UC(my_graph, 0.1);
  GeneralCascade::Build();
  cout << "Graph: " << Graph::GetN() << " " << Graph::GetM() << endl;

  char outfile[100];
  long time;
  double influence;
  int *seeds;

  // seeds = new int[2];
  // seeds[0] = 0, seeds[1] = 1;
  // cout << toSimulateOnce(seeds, 2, GeneralCascade::Run) << endl;
  // return 0;

  string s; 
  s="bsg";//basic static greedy

  if(!s.compare(algo)){
    sprintf(outfile, "BasicStaticGreedy_R%d_k%d.txt", R, set_size);

    int start_time = clock();
    timer tt;
    seeds = BasicStaticGreedy::GetSeeds(R, set_size);
    cout << "total time: " << tt.stop() << endl;
    int end_time = clock();
    time = end_time - start_time;

    // influence = toSimulateOnce(seeds, set_size, GeneralCascade::Run);
  }

  s="sgd";//static greedy with dynamic update on directed graph
  if(!s.compare(algo)){
    sprintf(outfile, "StaticGreedyUD_SGD_R%d_k%d.txt", R, set_size);
    
    int start_time = clock();
    seeds = StaticGreedyDirectedNew::GetSeedsForDirectedG(R, set_size);
    int end_time = clock();
    time = end_time - start_time;

    influence = toSimulateOnce(seeds, set_size, GeneralCascade::Run);
  }

  //output result
  printf("BasicStaticGreedy config: R=%d\n", R);
  printf("Time-consuming: T=%f sec\n", (double)(time)/CLOCKS_PER_SEC);
  printf("Influence-spread: I=%f\n",influence);
  printf("Seeds:\n");
  for (int i = 0; i < set_size; i++) {
      printf("%d ", seeds[i]);
  }
  printf("\n");

  return 1;
}
