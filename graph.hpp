#pragma once

#include <fcntl.h>
#include <malloc.h>
#include <parlay/parallel.h>
#include <parlay/primitives.h>
#include <parlay/sequence.h>
#include <parlay/random.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fstream>
#include <vector>
#include <map>

#include "utilities.h"
#include "get_time.hpp"
using namespace std;
using namespace parlay;

struct edge {
  NodeId u;
  NodeId v;
  edge() : u(0), v(0) {}
  edge(NodeId f, NodeId s) : u(f), v(s) {}
  bool operator<(const edge& rhs) const {
    if (u != rhs.u) {
      return u < rhs.u;
    } else {
      return v < rhs.v;
    }
  }
  bool operator!=(const edge& rhs) const { return u != rhs.u || v != rhs.v; }
};

struct Graph {
  size_t n;
  size_t m;
  bool symmetric;
  sequence<EdgeId> offset;
  sequence<NodeId> E;
  sequence<EdgeId> in_offset;
  sequence<NodeId> in_E;
  sequence<float> W;
  sequence<float> in_W;
};

// For spanning forest
struct Graph2 {
  size_t n;
  size_t m;
  sequence<NodeId> offset;
  sequence<NodeId> E;
};

struct Forest {
  size_t num_trees;
  Graph2 G;
  sequence<NodeId> vertex;
  sequence<NodeId> offset;
};

bool is_space(char c) {
  switch (c) {
    case '\r':
    case '\t':
    case '\n':
    case ' ':
    case 0:
      return true;
    default:
      return false;
  }
}

Graph read_pbbs(const char* const filename) {
  ifstream file(filename, ifstream::in | ifstream::binary | ifstream::ate);
  if (!file.is_open()) {
    cerr << "Error: Cannot open file " << filename << '\n';
    abort();
  }
  long end = file.tellg();
  file.seekg(0, file.beg);
  long length = end - file.tellg();

  sequence<char> strings(length + 1);
  file.read(strings.begin(), length);
  file.close();

  sequence<bool> flag(length);
  parallel_for(0, length, [&](size_t i) {
    if (is_space(strings[i])) {
      strings[i] = 0;
    }
  });
  flag[0] = strings[0];
  parallel_for(1, length, [&](size_t i) {
    if (is_space(strings[i - 1]) && !is_space(strings[i])) {
      flag[i] = true;
    } else {
      flag[i] = false;
    }
  });

  auto offset_seq = pack_index(flag);
  sequence<char*> words(offset_seq.size());
  parallel_for(0, offset_seq.size(),
               [&](size_t i) { words[i] = strings.begin() + offset_seq[i]; });

  Graph graph;
  size_t n = atol(words[1]);
  size_t m = atol(words[2]);
  graph.n = n;
  graph.m = m;
  graph.offset = sequence<EdgeId>(n + 1);
  graph.E = sequence<NodeId>(m);
  parallel_for(0, n, [&](size_t i) { graph.offset[i] = atol(words[i + 3]); });
  graph.offset[n] = m;
  parallel_for(0, m, [&](size_t i) { graph.E[i] = atol(words[n + i + 3]); });
  return graph;
}

Graph read_binary(const char* const filename, bool enable_mmap = true) {
  Graph graph;
  if (enable_mmap) {
    struct stat sb;
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
      cerr << "Error: Cannot open file " << filename << '\n';
      abort();
    }
    if (fstat(fd, &sb) == -1) {
      cerr << "Error: Unable to acquire file stat" << filename << '\n';
      abort();
    }
    void* memory = mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    char* data = static_cast<char*>(memory);
    size_t len = sb.st_size;
    size_t n = reinterpret_cast<uint64_t*>(data)[0];
    size_t m = reinterpret_cast<uint64_t*>(data)[1];
    size_t sizes = reinterpret_cast<uint64_t*>(data)[2];
    assert(sizes == (n + 1) * 8 + m * 4 + 3 * 8);
    graph.n = n, graph.m = m;
    graph.offset = sequence<EdgeId>(n + 1);
    graph.E = sequence<NodeId>(m);
    parallel_for(0, n + 1, [&](size_t i) {
      graph.offset[i] = reinterpret_cast<uint64_t*>(data + 3 * 8)[i];
    });
    parallel_for(0, m, [&](size_t i) {
      graph.E[i] = reinterpret_cast<uint32_t*>(data + 3 * 8 + (n + 1) * 8)[i];
    });

    if (memory != MAP_FAILED) {
      if (munmap(memory, len) != 0) {
        cerr << "Error: munmap failed" << '\n';
        abort();
      }
    }
  } else {
    ifstream ifs(filename);
    if (!ifs.is_open()) {
      cerr << "Error: Cannot open file " << filename << '\n';
      abort();
    }
    size_t n, m, sizes;
    ifs.read(reinterpret_cast<char*>(&n), sizeof(size_t));
    ifs.read(reinterpret_cast<char*>(&m), sizeof(size_t));
    ifs.read(reinterpret_cast<char*>(&sizes), sizeof(size_t));
    assert(sizes == (n + 1) * 8 + m * 4 + 3 * 8);

    graph.n = n;
    graph.m = m;
    sequence<uint64_t> offset(n + 1);
    sequence<uint32_t> edge(m);
    ifs.read(reinterpret_cast<char*>(offset.begin()), (n + 1) * 8);
    ifs.read(reinterpret_cast<char*>(edge.begin()), m * 4);
    graph.offset = sequence<EdgeId>(n + 1);
    graph.E = sequence<NodeId>(m);
    parallel_for(0, n + 1, [&](size_t i) { graph.offset[i] = offset[i]; });
    parallel_for(0, m, [&](size_t i) { graph.E[i] = edge[i]; });
    if (ifs.peek() != EOF) {
      cerr << "Error: Bad data\n";
      abort();
    }
    ifs.close();
  }
  return graph;
}

void make_symmetric(Graph& graph) {
  sequence<edge> edge_list(2 * graph.m);
  parallel_for(0, graph.n, [&](long i) {
    parallel_for(graph.offset[i], graph.offset[i + 1], [&](long j) {
      edge_list[j * 2] = edge(i, graph.E[j]);
      edge_list[j * 2 + 1] = edge(graph.E[j], i);
    });
  });
  sort_inplace(edge_list);
  auto unique_id = delayed_seq<bool>(edge_list.size(), [&](size_t i) {
    return (i == 0 || edge_list[i] != edge_list[i - 1]) &&
           edge_list[i].u != edge_list[i].v;
  });
  auto unique_edges = pack(edge_list, unique_id);
  edge_list.clear();

  graph.m = unique_edges.size();
  graph.E = sequence<NodeId>(graph.m);
  parallel_for(0, graph.n + 1, [&](size_t i) { graph.offset[i] = 0; });
  parallel_for(0, graph.m, [&](size_t i) {
    NodeId u = unique_edges[i].u;
    NodeId v = unique_edges[i].v;
    graph.E[i] = v;
    if (i == 0 || unique_edges[i - 1].u != u) {
      graph.offset[u] = i;
    }
    if (i == graph.m - 1 || unique_edges[i + 1].u != u) {
      size_t end = (i == graph.m - 1 ? (graph.n + 1) : unique_edges[i + 1].u);
      parallel_for(u + 1, end, [&](size_t j) { graph.offset[j] = i + 1; });
    }
  });
  graph.in_offset = graph.offset;
  graph.in_E = graph.E;
}

void make_directed(Graph& graph) {
  sequence<edge> edge_list(graph.m);
  parallel_for(0, graph.n, [&](size_t i) {
    parallel_for(graph.offset[i], graph.offset[i + 1],
                 [&](size_t j) { edge_list[j] = edge(graph.E[j], i); });
  });
  sort_inplace(edge_list);

  graph.in_offset = sequence<EdgeId>(graph.n + 1);
  graph.in_E = sequence<NodeId>(graph.m);
  parallel_for(0, graph.n + 1, [&](size_t i) { graph.in_offset[i] = 0; });
  parallel_for(0, graph.m, [&](size_t i) {
    NodeId u = edge_list[i].u;
    NodeId v = edge_list[i].v;
    graph.in_E[i] = v;
    if (i == 0 || edge_list[i - 1].u != u) {
      graph.in_offset[u] = i;
    }
    if (i == graph.m - 1 || edge_list[i + 1].u != u) {
      size_t end = (i == graph.m - 1 ? (graph.n + 1) : edge_list[i + 1].u);
      parallel_for(u + 1, end, [&](size_t j) { graph.in_offset[j] = i + 1; });
    }
  });
}

sequence<edge> StringToEdgeList(sequence<char> strings, long length){
  sequence<bool> flag(length);
  parallel_for(0, length, [&](size_t i) {
    if (is_space(strings[i])) {
      strings[i] = 0;
    }
  });
  flag[0] = strings[0];
  parallel_for(1, length, [&](size_t i) {
    if (is_space(strings[i - 1]) && !is_space(strings[i])) {
      flag[i] = true;
    } else {
      flag[i] = false;
    }
  });

  auto offset_seq = pack_index(flag);
  sequence<char*> words(offset_seq.size());
  parallel_for(0, offset_seq.size(),
               [&](size_t i) { words[i] = strings.begin() + offset_seq[i]; });
  
  EdgeId m = words.size()/2;
  sequence<edge> edge_list = sequence<edge>(m);
  parallel_for(0, m, [&](size_t i) {
    NodeId u = atol(words[2 * i]);
    NodeId v = atol(words[2 * i + 1]);
    edge_list[i] = edge(u, v);
  });
  return edge_list;
}

Graph EdgeListToGraph(sequence<edge> edge_list, NodeId n){
  EdgeId m = edge_list.size();
  Graph graph;
  graph.n = n;
  graph.m = m;
  bool reorder = false;
  parallel_for (0, m, [&](size_t i){
    NodeId u = edge_list[i].u;
    NodeId v = edge_list[i].v;
    if (u >= n || v >= n){
      reorder = true;
    }
  });
  if (reorder){
    cout << "Vertices need remapping" << endl;
    std::map<NodeId, NodeId> V_map;
    NodeId v_cnt = 0;
    for (size_t i = 0; i<m; i++){
      NodeId u = edge_list[i].u;
      NodeId v = edge_list[i].v;
      if (V_map.find(u)==V_map.end()){
        V_map[u]=v_cnt;
        v_cnt++;
      }
      if (V_map.find(v)==V_map.end()){
        V_map[v]=v_cnt;
        v_cnt++;
      }
    }
    sequence<edge> new_edgelist = sequence<edge>(m);
    parallel_for(0,m,[&](size_t i){
      NodeId u = edge_list[i].u;
      NodeId v = edge_list[i].v;
      new_edgelist[i] = edge(V_map[u], V_map[v]);
    });
    edge_list = new_edgelist;
    graph.n = v_cnt;
  }
  sort_inplace(edge_list);



  graph.offset = sequence<EdgeId>(graph.n + 1);
  graph.E = sequence<NodeId>(graph.m);
  parallel_for(0, graph.n + 1, [&](size_t i) { graph.offset[i] = 0; });
  parallel_for(0, graph.m, [&](size_t i) {
    NodeId u = edge_list[i].u;
    NodeId v = edge_list[i].v;
    graph.E[i] = v;
    if (i == 0 || edge_list[i - 1].u != u) {
      graph.offset[u] = i;
    }
    if (i == graph.m - 1 || edge_list[i + 1].u != u) {
      size_t end = (i == graph.m - 1 ? (graph.n + 1) : edge_list[i + 1].u);
      parallel_for(u + 1, end, [&](size_t j) { graph.offset[j] = i + 1; });
    }
  });

  return graph;
}


void read_knn(const char* const fileName, const char* const OutFileName,
              int k) {
  ifstream file(fileName, ifstream::in | ifstream::binary | ifstream::ate);
  if (!file.is_open()) {
    cerr << "Error: Cannot open file " << fileName << '\n';
    abort();
  }
  long end = file.tellg();
  file.seekg(0, file.beg);
  long length = end - file.tellg();

  sequence<char> strings(length + 1);
  file.read(strings.begin(), length);
  file.close();
  auto edge_list = StringToEdgeList(strings, length);
  size_t m = edge_list.size();
  size_t n = m / k;
  Graph graph = EdgeListToGraph(edge_list, n);

  std::string out_name = OutFileName;
  out_name += ".bin";  // C++11 for std::to_string   ofstream ofs(fileName);
  cout << "write to file "<<out_name << endl;
  cout <<"n: " << graph.n << " m: " << graph.m << endl;
  ofstream ofs(out_name);
  size_t sizes = (graph.n + 1) * 8 + graph.m * 4 + 3 * 8;
  ofs.write(reinterpret_cast<char*>(&graph.n), sizeof(size_t));
  ofs.write(reinterpret_cast<char*>(&graph.m), sizeof(size_t));
  ofs.write(reinterpret_cast<char*>(&sizes), sizeof(size_t));
  ofs.write(reinterpret_cast<char*>((graph.offset).data()), (graph.n + 1) * 8);
  ofs.write(reinterpret_cast<char*>((graph.E).data()), graph.m * 4);
  ofs.close();

  // symmetric graph
  make_symmetric(graph);
  out_name = OutFileName;
  out_name += "_sym.bin";  // C++11 for std::to_string   ofstream ofs(fileName);
  cout << "output to file " << out_name << endl;
  cout << "n: " << n << " m: " << m << endl;
  ofstream ofs_sym(out_name);
  sizes = (graph.n + 1) * 8 + graph.m * 4 + 3 * 8;
  ofs_sym.write(reinterpret_cast<char*>(&graph.n), sizeof(size_t));
  ofs_sym.write(reinterpret_cast<char*>(&graph.m), sizeof(size_t));
  ofs_sym.write(reinterpret_cast<char*>(&sizes), sizeof(size_t));
  ofs_sym.write(reinterpret_cast<char*>((graph.offset).data()),(graph.n + 1) * 8);
  ofs_sym.write(reinterpret_cast<char*>((graph.E).data()), graph.m * 4);
  ofs_sym.close();
}

void read_txt(const char* const fileName, const char* const OutFileName, NodeId n){
  ifstream file(fileName, ifstream::in | ifstream::binary | ifstream::ate);
  
  if (!file.is_open()) {
    cerr << "Error: Cannot open file " << fileName << '\n';
    abort();
  }
  long end = file.tellg();
  file.seekg(0, file.beg);
  // ignore lines begin with #
  long begin = file.tellg();
	while (!file.eof()) {
    string line;
	  getline(file,line);
	  if (line[0] == '#'){
      // cout << line << endl;
      begin = file.tellg();
    }
	  else{
      file.seekg(begin, file.beg);
      break;
    }
	}
  long length = end - begin;
  sequence<char> strings(length+1);
  file.read(strings.begin(), length);
  file.close();
  auto edge_list = StringToEdgeList(strings, length);
  Graph graph = EdgeListToGraph(edge_list, n);
  

  string out_name = OutFileName;
  out_name+=".bin";
  cout << "write to " << out_name << endl;
  cout << "n: " << graph.n << " m: " << graph.m << endl;
  ofstream ofs(out_name);
  size_t sizes = (graph.n + 1) * 8 + graph.m * 4 + 3 * 8;
  ofs.write(reinterpret_cast<char*>(&graph.n), sizeof(size_t));
  ofs.write(reinterpret_cast<char*>(&graph.m), sizeof(size_t));
  ofs.write(reinterpret_cast<char*>(&sizes), sizeof(size_t));
  ofs.write(reinterpret_cast<char*>((graph.offset).data()), (graph.n + 1) * 8);
  ofs.write(reinterpret_cast<char*>((graph.E).data()), graph.m * 4);
  ofs.close();


  
  // symmetric graph
  make_symmetric(graph);
  out_name = OutFileName;
  out_name += "_sym.bin";  // C++11 for std::to_string   ofstream ofs(fileName);
  cout << "write to " << out_name << endl;
  cout << "n: "<< graph.n << " m: " << graph.m << endl;
  ofstream ofs_sym(out_name);
  sizes = (graph.n + 1) * 8 + graph.m * 4 + 3 * 8;
  ofs_sym.write(reinterpret_cast<char*>(&graph.n), sizeof(size_t));
  ofs_sym.write(reinterpret_cast<char*>(&graph.m), sizeof(size_t));
  ofs_sym.write(reinterpret_cast<char*>(&sizes), sizeof(size_t));
  ofs_sym.write(reinterpret_cast<char*>((graph.offset).data()),(graph.n + 1) * 8);
  ofs_sym.write(reinterpret_cast<char*>((graph.E).data()), graph.m * 4);
  ofs_sym.close();
}

Graph generate_synthetic_graph(size_t n, size_t k) {
  Graph graph;
  graph.n = n;
  graph.m = n * k;
  printf("n=%zu, m=%zu\n", graph.n, graph.m);
  graph.symmetric = true;
  graph.offset = sequence<EdgeId>(graph.n + 1);
  graph.E = sequence<NodeId>(graph.m);
  auto perm = random_permutation(graph.n);
  parallel_for(0, n + 1, [&](size_t i) {
    graph.offset[i] = i * k;
  });
  parallel_for(0, n, [&](size_t i) {
    NodeId u = perm[i];
    assert(graph.offset[u + 1] - graph.offset[u] == k);
    for(size_t j = graph.offset[u]; j < graph.offset[u + 1]; j++) {
      graph.E[j] = perm[(i + 1 + j - graph.offset[u]) % n];
    }
  });
  return graph;
}

Graph read_graph(char* filename, bool enable_mmap = true,
                 bool symmetric = false) {
  if(strcmp(filename, "synthetic") == 0) {
    size_t n = 3563602789, k = 63;
    return generate_synthetic_graph(n, k);
  }
  //cout << "enable_mmap " << enable_mmap << endl;
  //cout << "symmetric " << symmetric << endl;
  string str_filename = string(filename);
  size_t idx = str_filename.find_last_of('.');
  if (idx == string::npos) {
    cerr << "Error: No graph extension provided\n";
    abort();
  }
  string subfix = str_filename.substr(idx + 1);
  Graph graph;
  if (subfix == "adj") {
    graph = read_pbbs(filename);
  } else if (subfix == "bin") {
    graph = read_binary(filename, enable_mmap);
  } else {
    cerr << "Error: Invalid graph extension\n";
  }
  if (str_filename.find("sym") != string::npos) {
    // graph.in_offset = graph.offset;
    // graph.in_E = graph.E;
    graph.symmetric = true;
  } else if (symmetric) {
    make_symmetric(graph);
    graph.symmetric = true;
  } else {
    graph.symmetric = false;
    make_directed(graph);
  }
  return graph;
}

Graph read_large_graph(char* filename) {
  ifstream ifs(filename);
  if (!ifs.is_open()) {
    cerr << "Error: Cannot open file " << filename << '\n';
    abort();
  }
  size_t n, m, sizes;
  ifs.read(reinterpret_cast<char*>(&n), sizeof(size_t));
  ifs.read(reinterpret_cast<char*>(&m), sizeof(size_t));
  ifs.read(reinterpret_cast<char*>(&sizes), sizeof(size_t));
  assert(sizes == (n + 1) * 8 + m * 4 + 3 * 8);

  Graph graph;
  graph.n = n;
  graph.m = m;
  graph.offset = sequence<EdgeId>(n + 1);
  graph.E = sequence<NodeId>(m);
  ifs.read(reinterpret_cast<char*>(graph.offset.begin()), (n + 1) * 8);
  ifs.read(reinterpret_cast<char*>(graph.E.begin()), m * 4);

  ifs.read(reinterpret_cast<char*>(&n), sizeof(size_t));
  ifs.read(reinterpret_cast<char*>(&m), sizeof(size_t));
  ifs.read(reinterpret_cast<char*>(&sizes), sizeof(size_t));
  assert(n == graph.n);
  assert(m == graph.m);

  graph.in_offset = sequence<EdgeId>(n + 1);
  graph.in_E = sequence<NodeId>(m);
  ifs.read(reinterpret_cast<char*>(graph.in_offset.begin()), (n + 1) * 8);
  ifs.read(reinterpret_cast<char*>(graph.in_E.begin()), m * 4);
  if (ifs.peek() != EOF) {
    cerr << "Error: Bad data\n";
    abort();
  }
  ifs.close();
  return graph;
}

Graph read_large_sym_graph(char* filename) {
  ifstream ifs(filename);
  if (!ifs.is_open()) {
    cerr << "Error: Cannot open file " << filename << '\n';
    abort();
  }
  size_t n, m, sizes;
  ifs.read(reinterpret_cast<char*>(&n), sizeof(size_t));
  ifs.read(reinterpret_cast<char*>(&m), sizeof(size_t));
  ifs.read(reinterpret_cast<char*>(&sizes), sizeof(size_t));
  assert(sizes == (n + 1) * 8 + m * 4 + 3 * 8);

  Graph graph;
  graph.n = n;
  graph.m = m;
  graph.offset = sequence<EdgeId>(n + 1);
  graph.E = sequence<NodeId>(m);
  ifs.read(reinterpret_cast<char*>(graph.offset.begin()), (n + 1) * 8);
  ifs.read(reinterpret_cast<char*>(graph.E.begin()), m * 4);
  graph.symmetric = true;

  if (ifs.peek() != EOF) {
    cerr << "Error: Bad data\n";
    abort();
  }
  ifs.close();
  return graph;
}

// void write_array_to_csv(char* filename, sequence<NodeId>& arr, size_t n) {
//   char* csv = (char*)".csv";
//   char csvFile[strlen(csv) + strlen(filename) + 1];
//   csvFile[0] = '\0';
//   strcat(csvFile, filename);
//   strcat(csvFile, csv);

//   ofstream file(csvFile, ios::out | ios::binary);
//   if (!file.is_open()) {
//     std::cout << "Unable to open file: " << csvFile << std::endl;
//     abort();
//   }
//   cout << "write to csv file: " << csvFile << endl;
//   file << "ldd label" << endl;
//   char buffer[50];
//   for (size_t i = 0; i < n; i++) {
//     sprintf(buffer, "%u", arr[i]);
//     file << buffer << endl;
//   }

//   file.close();
// }


struct Hash_Edge {
    EdgeId hash_graph_id;
    // NodeId n;
    // NodeId id_term;
    // bool forward;
    bool operator()(NodeId u, NodeId v, float w) const {
      return (hash_graph_id ^ _hash(((EdgeId)min(u,v)<< 32) + max(u,v))) < UINT_E_MAX*w;
      // if (!forward) swap(u,v);
      // return _hash(_hash(u)+v)+_hash(graph_id) < w*UINT_N_MAX;
      // return _hash( graph_id*n*n + u*n + v) < w*UINT_N_MAX;
      // return _hash(id_term + u*n + v) < w*UINT_N_MAX;

    }
};


void AssignUniWeight(Graph& graph, float w){
  graph.W = sequence<float>(graph.m);
  parallel_for(0, graph.m, [&](size_t i){
    graph.W[i] = w;
  });
  if (!graph.symmetric){
    graph.in_W = sequence<float>(graph.m);
    parallel_for(0, graph.m, [&](size_t i){
      graph.in_W[i]=w;
    });
  }
}

void AssignIndegreeWeightSym(Graph &graph){
  graph.W = sequence<float>(graph.m);
  parallel_for(0, graph.n, [&](size_t i){
    EdgeId in_degree = graph.offset[i+1] - graph.offset[i];
    if (in_degree >0){
      float w = 1.0/in_degree;
      parallel_for(graph.offset[i],graph.offset[i+1],[&](size_t j){
        graph.W[j] = w;
      });
    }
  });
}

void AssignIndegreeWeightDir(Graph &graph){
  graph.in_W = sequence<float>(graph.m);
  graph.W = sequence<float>(graph.m);
  sequence<tuple<edge,float>> edge_list = sequence<tuple<edge,float>>(graph.m);
  parallel_for(0, graph.n, [&](size_t i){
    EdgeId in_degree = graph.in_offset[i+1] - graph.in_offset[i];
    if (in_degree >0){
      float w = 1.0/in_degree;
      parallel_for(graph.in_offset[i], graph.in_offset[i+1], [&](size_t j){
        graph.in_W[j]=w;
        NodeId u = graph.in_E[j];
        edge_list[j] = make_tuple(edge(u,i),w);
      });
    }
  });
  sort_inplace(edge_list);
  parallel_for(0,graph.m, [&](size_t i){
    graph.W[i]=get<1>(edge_list[i]);
  });
}

void AssignIndegreeWeight(Graph &graph){
  if (graph.symmetric){
    AssignIndegreeWeightSym(graph);
  }else{
    AssignIndegreeWeightDir(graph);
  }
}

// void WriteSampledGraph(Graph graph, const char* const OutFileName){
//   sequence<edge> edge_list = sequence<edge>(graph.m);
//   timer t;
//   t.start();
//   parallel_for(0, graph.n, [&](size_t i){
//     NodeId u = i;
//     parallel_for(graph.offset[i], graph.offset[i+1], [&](size_t j){
//       NodeId v = graph.E[j];
//       edge_list[j] = edge(u,v);
//     });
//   });
//   t.stop();
//   cout << "traverse edges costs: " << t.get_total()<<endl;

//   Hash_Edge hash_edge{0, (NodeId)graph.n ,true};
//   auto sample_edges = delayed_seq<bool>(edge_list.size(), [&](size_t i) {
//     NodeId u = edge_list[i].u;
//     NodeId v = edge_list[i].v;
//     float w = graph.W[i];
//     return hash_edge(u,v,w);
//   });
//   auto unique_edges = pack(edge_list, sample_edges);
//   edge_list.clear();
//   Graph graph_sampled = EdgeListToGraph(unique_edges, graph.n);

//   string out_name = OutFileName;
//   out_name+=".bin";
//   cout << "write to " << out_name << endl;
//   cout << "n: " << graph_sampled.n << " m: " << graph_sampled.m << endl;
//   ofstream ofs(out_name);
//   size_t sizes = (graph_sampled.n + 1) * 8 + graph_sampled.m * 4 + 3 * 8;
//   ofs.write(reinterpret_cast<char*>(&graph_sampled.n), sizeof(size_t));
//   ofs.write(reinterpret_cast<char*>(&graph_sampled.m), sizeof(size_t));
//   ofs.write(reinterpret_cast<char*>(&sizes), sizeof(size_t));
//   ofs.write(reinterpret_cast<char*>((graph_sampled.offset).data()), (graph_sampled.n + 1) * 8);
//   ofs.write(reinterpret_cast<char*>((graph_sampled.E).data()), graph_sampled.m * 4);
//   ofs.close();
// }