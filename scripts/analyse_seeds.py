import os
import re
import sys
import subprocess
import graph
import numpy as np

IM_DIR = os.path.dirname(os.path.abspath(__file__))
# INFUSER_DIR = os.path.dirname(os.path.abspath(__file__)) + '/../../infuser'

def analyse_im_seeds(graph, w, folder):
    g = graph.split('/')[-1]
    file_in = f'{IM_DIR}/{folder}/{g[:-4]}.txt'
    file_out = f'{IM_DIR}/{folder}/seeds_{g[:-4]}.txt'
    print(f'anaylyse IM_compact on {file_in}')
    # command = f'{IM_DIR}/../general_cascade {graph} {file_in} -w {w} -i 20000'
    command = f'{IM_DIR}/../general_cascade {graph} {file_in} -w {w} -i 2000'
    subprocess.call(f'numactl -i all {command} >> {file_out}', shell=True)
            
    


def analyse_im_seeds_all(folder):
    for g in graph.get_small_graphs():
        analyse_im_seeds(g, 0.02, folder)
    for g in graph.get_large_graphs():
        analyse_im_seeds(g, 0.02, folder)
    for g in graph.get_road_graphs():
        analyse_im_seeds(g, 0.2,folder)
    for g in graph.get_knn_graphs():
        analyse_im_seeds(g, 0.2,folder)
    for g in graph.get_grid_graphs():
        analyse_im_seeds(g, 0.2, folder)

def collect_influence(graph, folder):
    g = graph.split("/")[-1]
    file_in = f'{IM_DIR}/{folder}/seeds_{g[:-4]}.txt'
    print(file_in)
    offset_l = 1
    block_l = 4
    f_in =open(file_in,'r')
    lines = f_in.readlines()
    f_in.close()
    n = (len(lines)-offset_l)// block_l
    influence = []
    for i in range(1,n+1):
        score = lines[offset_l+i*block_l-1].replace("\n","")
        influence.append(score)
    return influence

def collect_influence_all(folder, file_out):
    graphs = graph.get_all_graphs()
    influence = {}
    for g in graphs:
        print(g)
        g_short = g.split("/")[-1]
        influence[g_short]=collect_influence(g, folder)
    print(influence.keys())
    file_out = f"{IM_DIR}/{file_out}.txt"
    with open(file_out, 'a') as f:
        f.write("influence score\n")
        for g in influence.keys():
            f.write(f"{graph.graphs_map[g]} ")
            f.write(" ".join(influence[g]))
            f.write("\n")
 
def collect_memory(graph, folder):
    g = graph.split('/')[-1]
    # compact_list = [1, 0.5, 0.25, 0.1, 0.05]
    data_memory=[]
    # for c in compact_list:
        # suffix = str(int(c*100))
    file_in = f'{IM_DIR}/{folder}/{g[:-4]}_mem.txt'
    f = open(file_in, 'r')
    mem = f.read()
    f.close()
    memory_kb = re.findall('Maximum resident set size.*', mem)
    # assert len(memory_kb) == len(graphs)
    memory_kb = list(map(lambda x: int(x.split(' ')[-1]), memory_kb))
    memory_gb = list(map(lambda x: x / 1000000.0, memory_kb))
    # for i in range(3-len(memory_gb)):
    #     memory_gb.append("NA")
    # data_memory.append(memory_gb)
    # a = np.array(data_memory)
    # a = a.T
    # return a.reshape(1,15)[0]
    return memory_gb
def collect_memory_all(folder, file_name):
    data = {}
    graphs = graph.get_all_graphs()
    for g in graphs:
        g_short = g.split('/')[-1]
        data[g_short]=collect_memory(g, folder)
    print(data.keys())
    file_out = f"{IM_DIR}/{file_name}.txt"
    with open(file_out, 'a') as f:
        f.write("Memory Usage\n")
        for g in data.keys():
            lines = []
            lines.append(graph.graphs_map[g])
            for l in data[g]:
                lines.append(str(l))
            lines.append('\n')
            f.write(' '.join(lines))

def collect_time(graph, folder):
    g = graph.split('/')[-1]
    # compact_list = [1, 0.5, 0.25, 0.1, 0.05]
    # data_total=[]
    # data_select=[]
    # for c in compact_list:
        # suffix = str(int(c*100))
    file_in = f'{IM_DIR}/{folder}/{g[:-4]}.txt'
    f = open(file_in, 'r')
    res = f.read()
    f.close()
    time_map = dict()
    allocate_time = re.findall('allocating sketching memory time:.*', res)
    time_map["allocate_time"] = list(map(lambda x: eval(x.split(' ')[-1]), allocate_time))
    union_time = re.findall('union time time:.*',res)
    time_map["union_time"] = list(map(lambda x: eval(x.split(' ')[-1]), union_time))
    sort_time = re.findall('sort time:.*',res)
    time_map["sort_time"] = list(map(lambda x: eval(x.split(' ')[-1]), sort_time))
    write_time = re.findall("write sketch time:.*", res)
    time_map["write_time"] = list(map(lambda x: eval(x.split(" ")[-1]), write_time))
    Q_time = re.findall("construct priority queue time:.*", res)
    time_map["Q_time"] = list(map(lambda x: eval(x.split(" ")[-1]), Q_time))
    seed_time = re.findall("computing selection time:.*", res)
    time_map["seed_time"] = list(map(lambda x: eval(x.split(" ")[-1]), seed_time))

    sketch_time = re.findall("sketch construction time:.*", res)
    time_map["sketch_time"] = list(map(lambda x: eval(x.split(" ")[-1]), sketch_time))
    selection_time = re.findall("seed selection time:.*", res)
    time_map["selection_time"] = list(map(lambda x: eval(x.split(" ")[-1]), selection_time))
    total_time = re.findall("total time:.*", res)
    time_map["total_time"] = list(map(lambda x: eval(x.split(" ")[-1]), total_time))
    return time_map
def collect_time_all(folder, file_name, graphs):
    time_map=dict()
    # graphs = graph.get_all_graphs()
    for g in graphs:
        g_short = g.split('/')[-1]
        time_map[g_short] =collect_time(g,folder)
    graph_keys = list(time_map.keys())
    print(graph_keys)
    item_keys = list(time_map[graph_keys[0]].keys())
    file_out = f"{IM_DIR}/{file_name}.txt"
    with open(file_out, 'a') as f:
        for item in item_keys:
            f.write(f"{item}\n")
            for g in graph_keys:
                lines = []
                lines.append(graph.graphs_map[g])
                for l in time_map[g][item]:
                    lines.append(str(l))
                lines.append('\n')
                f.write(' '.join(lines))

def collect_evalues(graph, folder):
    g = graph.split('/')[-1]
    file_in = f'{IM_DIR}/{folder}/{g[:-4]}.txt'
    f = open(file_in, 'r')
    mem = f.read()
    f.close()
    num_evals = re.findall('total re-evaluate times:.*', mem)
    # assert len(memory_kb) == len(graphs)
    num_evals = list(map(lambda x: x.split(' ')[-1], num_evals))
    return num_evals
def collect_evalues_all(folder,file_name):
    data = {}
    graphs = graph.get_all_graphs()
    for g in graphs:
        g_short = g.split('/')[-1]
        data[g_short]=collect_evalues(g, folder)
    print(data.keys())
    file_out = f"{IM_DIR}/{file_name}.txt"
    with open(file_out, 'a') as f:
        f.write("Number of re-evaluations\n")
        for g in data.keys():
            lines = []
            lines.append(graph.graphs_map[g])
            for l in data[g]:
                lines.append(str(l))
            lines.append('\n')
            f.write(' '.join(lines))

def collect_seeds(graph, folder):
    g = graph.split('/')[-1]
    file_in = f'{IM_DIR}/{folder}/{g[:-4]}.txt'
    f = open(file_in, 'r')
    mem = f.read()
    f.close()
    num_evals = re.findall('seeds:.*', mem)
    return num_evals
def collect_seeds_all(folder,file_name):
    data = {}
    graphs = graph.get_all_graphs()
    for g in graphs:
        g_short = g.split('/')[-1]
        data[g_short]=collect_seeds(g, folder)
    print(data.keys())
    file_out = f"{IM_DIR}/{file_name}.txt"
    with open(file_out, 'a') as f:
        f.write("Number of re-evaluations\n")
        for g in data.keys():
            f.write(f'{graph.graphs_map[g]}\n')
            for l in data[g]:
                f.write(f'{l}\n')

def move_graph():
    graphs = graph.get_small_graphs()+ graph.get_large_graphs()\
            +graph.get_knn_graphs()+graph.get_grid_graphs()
    print(graphs)
    for g in graphs:
        subprocess.call(f'scp {g} xe-01:/home/csgrads/lwang323/data/', shell=True)

def compare_seeds(file_name1, file_name2):
    seeds1 = []
    seeds2 = []
    file1 = open(file_name1,'r')
    file2 = open(file_name2,'r')
    f1 = file1.read()
    seeds1 = re.findall('seeds: .*', f1)
    f2 = file2.read()
    seeds2 = re.findall('seeds: .*', f2)
    n = min(len(seeds1), len(seeds2))
    for i in range(n):
        if (seeds1[i]!= seeds2[i]):
            print(f"{i} doesn't match\n")
            print(f"{file_name1} is : ")
            print(seeds1[i])
            print(f"{file_name2} is : ")
            print(seeds2[i])


if __name__ == '__main__':
    folder = "hash"
    analyse_im_seeds_all(folder)
    # folders = ["stdQ2", "BST2", "WinTree2"]
    # for f in folders:
        # collect_time_all(f, f)
        # collect_memory_all(f,f)
    # collect_influence_all("R", "R")
    # analyse_memory_all(INFUSER_DIR)
    # graph_=graph.graph_dir+"Epinions1_sym_1.txt"
    # g = "Epinions1_sym_1.txt"
    # file_in = f'{IM_DIR}/scripts/infuser_paper/{g[:-4]}_our.txt'
    # file_out = f'{IM_DIR}/scripts/infuser_paper/seeds_{g[:-4]}_our.txt'
    # print(f'anaylyse IM_compact on {file_in}')
    # command = f'{IM_DIR}/general_cascade {graph_} {file_in} -w {0.01} -k 50 -i 20000'
    # subprocess.call(f'numactl -i all {command} >> {file_out}', shell=True)
    # analyse_im_seeds_all(INFUSER_DIR, "R_002_count")
