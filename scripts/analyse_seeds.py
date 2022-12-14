import os
import re
import sys
import subprocess
import graph


IM_DIR = os.path.dirname(os.path.abspath(__file__))
# INFUSER_DIR = os.path.dirname(os.path.abspath(__file__)) + '/../../infuser'

def analyse_im_seeds(graph, w, folder):
    g = graph.split('/')[-1]
    file_in = f'{IM_DIR}/{folder}/{g[:-4]}.txt'
    file_out = f'{repo}/{folder}/seeds_{g[:-4]}.txt'
    print(f'anaylyse IM_compact on {file_in}')
    command = f'{IM_DIR}/general_cascade {graph} {file_in} -w {w} -i 20000'
    subprocess.call(f'numactl -i all {command} >> {file_out}', shell=True)
            
    


def analyse_im_seeds_all(folder):
    for g in graph.get_small_graphs():
        analyse_im_seeds(g, 0.02, folder)
    for g in graph.get_large_graphs():
        analyse_im_seeds(g, 0.02, folder)
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
    file_in = f'{IM_DIR}/{folder}/{g[:-4]}_mem.txt'
    f = open(file_in, 'r')
    mem = f.read()
    f.close()
    memory_kb = re.findall('Maximum resident set size.*', mem)
    # assert len(memory_kb) == len(graphs)
    memory_kb = list(map(lambda x: int(x.split(' ')[-1]), memory_kb))

    memory_gb = list(map(lambda x: x / 1000000.0, memory_kb))
    return memory_gb
def collect_memory_all(folder, file_name):
    data = {}
    graphs = graph.get_all_graphs()
    for g in graphs:
        g_short = g.split('/')[-1]
        try:
            data[g_short]=collect_memory(g, folder)
        except:
            continue
    print(data.keys())
    file_out = f"{folder}/{file_name}.txt"
    with open(file_out, 'a') as f:
        for g in data.keys():
            lines = []
            lines.append(graph.graphs_map[g])
            for l in data[g]:
                lines.append(str(l))
            lines.append('\n')
            f.write(' '.join(lines))

def collect_time(graph, folder):
    g = graph.split('/')[-1]
    file_in = f'{IM_DIR}/{folder}/{g[:-4]}.txt'
    f = open(file_in, 'r')
    res = f.read()
    f.close()
    total_time = re.findall('total time:.*', res)
    total_time = list(map(lambda x: eval(x.split(' ')[-1]), total_time))
    first_round_time = re.findall('Till first round time:.*',res)
    first_round_time = list(map(lambda x: eval(x.split(' ')[-1]), first_round_time))
    return (total_time, first_round_time)

def collect_time_all(folder, file_name):
    fisrt_data = {}
    total_data = {}
    graphs = graph.get_all_graphs()
    for g in graphs:
        g_short = g.split('/')[-1]
        total_data[g_short],first_data[g_short] =collect_time(g,folder)
    print(total_data.keys())
    file_out = f"{IM_DIR}/{file_name}.txt"
    with open(file_out, 'a') as f:
        f.write("total running time\n")
        for g in total_data.keys():
            lines = []
            lines.append(graph.graphs_map[g])
            for l in total_data[g]:
                lines.append(str(l))
            lines.append('\n')
            f.write(' '.join(lines))
        f.write("first round time\n")
        for g in first_data.keys():
            lines = []
            lines.append(graph.graphs_map[g])
            for l in first_data[g]:
                lines.append(str(l))
            lines.append("\n")
            f.write(' '.join(lines))


def move_graph():
    graphs = graph.get_small_graphs()+ graph.get_large_graphs()\
            +graph.get_knn_graphs()+graph.get_grid_graphs()
    print(graphs)
    for g in graphs:
        subprocess.call(f'scp {g} xe-01:/home/csgrads/lwang323/data/', shell=True)

def compare_seeds(file_name1, file_name2):
    seeds1 = []
    seeds2 = []
    file1 = open(f'{IM_DIR}/scripts/infuser_R_002/{file_name1}.txt','r')
    file2 = open(f'{IM_DIR}/scripts/infuser_R_002/{file_name2}.txt','r')
    f1 = file1.read()
    seeds1 = re.findall('seeds: .*', f1)
    f2 = file2.read()
    seeds2 = re.findall('seeds: .*', f2)
    n = min(len(seeds1), len(seeds2))
    for i in range(n):
        if (seeds1[i]!= seeds2[i]):
            print(f"2^{i} doesn't match\n")
            print(f"{file1} is : ")
            print(seeds1[i])
            print(f"{file2} is : ")
            print(seeds2[i])


# if __name__ == '__main__':
    # analyse_memory_all(INFUSER_DIR)
    # graph_=graph.graph_dir+"Epinions1_sym_1.txt"
    # g = "Epinions1_sym_1.txt"
    # file_in = f'{IM_DIR}/scripts/infuser_paper/{g[:-4]}_our.txt'
    # file_out = f'{IM_DIR}/scripts/infuser_paper/seeds_{g[:-4]}_our.txt'
    # print(f'anaylyse IM_compact on {file_in}')
    # command = f'{IM_DIR}/general_cascade {graph_} {file_in} -w {0.01} -k 50 -i 20000'
    # subprocess.call(f'numactl -i all {command} >> {file_out}', shell=True)
    # analyse_im_seeds_all(INFUSER_DIR, "R_002_count")
