import os
import sys
import subprocess
import graph
import analyse_seeds


IM_DIR = os.path.dirname(os.path.abspath(__file__))



def run_im_compact(graph, folder, w):
    print(f'running IM_compact on {graph}')
    g = graph.split('/')[-1]
    file_out = f'{IM_DIR}/{folder}/{g[:-4]}.txt'
    mem_out = f'{IM_DIR}/{folder}/{g[:-4]}_mem.txt'
    for i in range(5, 16):
        R = 1<<i
        command = f'{IM_DIR}/../IM {graph} -k 100 -R {R} -w {w} -compact 1'
        # subprocess.call(f'numactl -i all {command} >> {file_out}', shell=True)
        subprocess.call(
        f'/usr/bin/time -v numactl -i all {command} 1>> {file_out} 2>> {mem_out}', shell=True)
    


def run_im_compact_all(folder):
    for g in graph.get_small_graphs():
        run_im_compact(g,folder ,0.02)
    for g in graph.get_large_graphs():
        run_im_compact(g, folder,0.02)
    for g in graph.get_knn_graphs():
        run_im_compact(g, folder,0.2)
    for g in graph.get_grid_graphs():
        run_im_compact(g, folder, 0.2)



if __name__ == '__main__':
    folder = 'IM_002'
    # run_im_compact_all(folder)
    # analyse_seeds.collect_time_all(folder, folder)
    # analyse_seeds.collect_memory_all(folder, folder)
    # analyse_seeds.analyse_im_seeds_all(folder)
    analyse_seeds.collect_influence_all(folder,folder)