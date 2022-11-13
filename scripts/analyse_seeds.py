import os
import re
import sys
import subprocess
import graph


IM_DIR = os.path.dirname(os.path.abspath(__file__)) + '/..'

def get_sym_real_graphs():
    return [
        graph.small_graph_dir+'HepPh_sym.bin',
        graph.small_graph_dir+'DBLP_sym.bin',
        graph.large_graph_dir+'soc-LiveJournal1_sym.bin',
        # graph.large_graph_dir+'sd_arc_sym.bin',
        graph.large_graph_dir+'RoadUSA_sym.bin',
        graph.large_graph_dir+'Germany_sym.bin',
    ]
def get_sym_syn_graphs():
    return [
        graph.knn_graph_dir+'CHEM_5_sym.bin',
        graph.knn_graph_dir+'GeoLifeNoScale_5_sym.bin',
        # graph.knn_graph_dir+'GeoLifeNoScale_15_sym.bin',
        graph.grid_graph_dir+'grid_4000_4000_sym.bin',
    ]

def analyse_im_seeds(graph, w=0.1):
    print(f'anaylyse IM_compact on {graph}')
    g = graph.split('/')[-1]
    file_in = f'{IM_DIR}/scripts/R_001/{g[:-4]}.txt'
    file_out = f'{IM_DIR}/scripts/R_001/seeds_{g[:-4]}.txt'
    command = f'{IM_DIR}/general_cascade {graph} {file_in} -w {w} -i 20000'
    subprocess.call(f'numactl -i all {command} | tee {file_out}', shell=True)
            
    


def analyse_im_seeds_all():
    # for g in graph.get_small_graphs():
    #     analyse_im_seeds(g, 0.01)
    for g in graph.get_large_graphs():
        analyse_im_seeds(g, 0.01)
    for g in graph.get_knn_graphs():
        analyse_im_seeds(g, 0.01)

def analyse_im_seeds_results(graph, w):
    g = graph.split('/')[-1]
    file_in = f'{IM_DIR}/scripts/R_001/seeds_{g[:-4]}.txt'
    K = [50,100,150,200]
    time = {}
    for k in K:
        time[k]=[]
    influences = []
    with open(file_in) as f:
        lines = f.readlines()
        for l in lines:
            if (' ' not in l):
                influences.append(eval(l))
    n = int(len(influences)/4)
    for i in range(4):
        time[K[i]] = influences[i*n: (i+1)*n]
    return time
def analyse_im_seeds_results_all():
    data = {}
    for g in graph.get_small_graphs():
        g_short = g.split('/')[-1]
        data[g_short]=analyse_im_seeds_results(g, 0.01)
    for g in graph.get_large_graphs():
        g_short = g.split('/')[-1]
        data[g_short]=analyse_im_seeds_results(g, 0.01)
    for g in graph.get_knn_graphs():
        g_short = g.split('/')[-1]
        data[g_short]=analyse_im_seeds_results(g, 0.01)
    print(data.keys())
    file_out = f"{IM_DIR}/scripts/R_001.txt"
    with open(file_out, 'w') as f:
        for k in [50,100,150,200]:
            f.write(f'{k}:\n')
            for g in data.keys():
                lines = []
                lines.append(g)
                for l in data[g][k]:
                    lines.append(str(l))
                lines.append('\n')
                f.write(' '.join(lines))            

def move_graph():
    graphs = graph.get_small_graphs()+ graph.get_large_graphs()\
            +graph.get_knn_graphs()+graph.get_grid_graphs()
    print(graphs)
    for g in graphs:
        subprocess.call(f'scp {g} xe-01:/home/csgrads/lwang323/data/', shell=True)
    

if __name__ == '__main__':
    # analyse_im_seeds_all()
    # analyse_im_seeds(graph.small_graph_dir+'HepPh_sym.bin', 0.01)
    # analyse_im_seeds_results_all()
    # analyse_im_seeds(graph.knn_graph_dir+'CHEM_5_sym.bin', 0.2)
    move_graph()
