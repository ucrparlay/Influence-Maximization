import os
import sys
import subprocess
import run_im
import re


IM_DIR = os.path.dirname(os.path.abspath(__file__)) + '/..'
INFUSER_DIR = os.path.dirname(os.path.abspath(__file__)) + '/../../infuser'

graphs = [
    'Epinions1_sym.bin',
    'Slashdot_sym.bin',
    'DBLP_sym.bin',
    'Youtube_sym.bin',
    'com-orkut_sym.bin',
    'soc-LiveJournal1_sym.bin',  
]

directed_graphs=[
 'Epinions1.bin',
 'Slashdot.bin',
 'soc-LiveJournal1.bin', 
]
graph_dir = "/home/csgrads/lwang323/data/"

def run_infuser(graph, w):
    print(f'running infuser on {graph}')
    g = graph.split('/')[-1]
    file_out = f'{IM_DIR}/scripts/infuser_paper/{g[:-4]}_{int(w*100)}.txt'
    mem_out = f'{IM_DIR}/scripts/infuser_paper/{g[:-4]}_{int(w*100)}_mem.txt'
    R = 2048
    subprocess.call(f'echo R {R} w {w} >> {file_out}',shell=True)
    # command = f'{IM_DIR}/bin/infuser -M HyperFuser -K 200 -R {R} -p {w} {graph}'
    command = f'{INFUSER_DIR}/bin/infuser -M infuser -K 50 -R {R} -p {w} {graph}'
    # subprocess.call(f'numactl -i all {command} >> {file_out}', shell=True)
    print(command)
    subprocess.call(
        f'/home/csgrads/lwang323/time-1.9/time -v numactl -i all {command} 1>> {file_out} 2>> {mem_out}', shell=True)

def collect_seeds(graph, w):
    g = graph.split('/')[-1]
    file_name = f'{IM_DIR}/scripts/infuser_paper/{g[:-4]}_{int(w*100)}.txt'
    print(f'collect seeds on {file_name}')
    Seeds_set = []
    with open(file_name) as f:
        lines = f.readlines()
        seeds_size = 52
        i = 0
        while (i*seeds_size+50 < len(lines)):
            seeds = []
            for j in range(seeds_size-2):
                words = lines[seeds_size*i+2+j].split("\t")
                seeds.append(words[0])
            Seeds_set.append(seeds)
            i+=1
    with open(file_name, 'a') as f:
        for seeds in Seeds_set:
            f.write("seeds: ")
            f.write(' '.join(seeds))
            f.write("\n")

def analyse_im_seeds(graph, w):
    g = graph.split('/')[-1]
    file_in = f'{IM_DIR}/scripts/infuser_paper/{g[:-4]}_{int(w*100)}.txt'
    file_out = f'{IM_DIR}/scripts/infuser_paper/seeds_{g[:-4]}_{int(w*100)}.txt'
    print(f'anaylyse influence on {file_in}')
    command = f'{IM_DIR}/general_cascade {graph[:-4]}_1.txt {file_in} -k 50 -w {w} -i 20000'
    subprocess.call(f'numactl -i all {command} >> {file_out}', shell=True)

def analyse_memory(graph, w):
    g = graph.split('/')[-1]
    file_in = f'{IM_DIR}/scripts/infuser_paper/{g[:-4]}_{int(w*100)}_mem.txt'
    f = open(file_in, 'r')
    mem = f.read()
    f.close()
    memory_kb = re.findall('Maximum resident set size.*', mem)
    # assert len(memory_kb) == len(graphs)
    memory_kb = list(map(lambda x: int(x.split(' ')[-1]), memory_kb))
    memory_gb = list(map(lambda x: x / 1000000.0, memory_kb))
    return memory_gb
if __name__ == '__main__':
    for g in graphs:
        for w in {0.1,0.01}:
            # run_infuser(graph_dir+g, w)
            # collect_seeds(graph_dir+g, w)
            # analyse_im_seeds(graph_dir+g, w) 
            print(f'{g} {w}')
            mem = analyse_memory(g,w)
            print(mem) 
    # collect_seeds(graph_dir+'Epinions1_sym.bin', 0.01)
    # analyse_im_seeds(graph_dir+'Epinions1_sym.bin',0.01)  