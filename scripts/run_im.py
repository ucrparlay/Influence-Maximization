import os
import sys
import subprocess
import graph


IM_DIR = os.path.dirname(os.path.abspath(__file__)) + '/..'



def run_im_compact(graph, w=0.1):
    print(f'running IM_compact on {graph}')
    g = graph.split('/')[-1]
    file_out = f'{IM_DIR}/scripts/R_005/{g[:-4]}.txt'
    for i in range(16):
        R = 1<<i
        command = f'{IM_DIR}/IM {graph} -k 200 -R {R} -w {w} -compact 1'
        subprocess.call(f'numactl -i all {command} >> {file_out}', shell=True)
    # mem_out = f'{IM_DIR}/scripts/im_compact_{compact}_mem.txt'
    # subprocess.call(
    #     f'/usr/bin/time -v numactl -i all {command} 1>> {file_out} 2>> {mem_out}', shell=True)
    


def run_im_compact_all():
    for g in graph.get_small_graphs():
        run_im_compact(g, 0.05)
    for g in graph.get_large_graphs():
        run_im_compact(g, 0.05)
    for g in graph.get_knn_graphs():
        run_im_compact(g, 0.5)
    for g in graph.get_grid_graphs():
        run_im_compact(g, 0.5)
    # for g in graph.get_sym_real_graphs():
    #     run_im_compact(g,0.1)
    # for g in graph.get_sym_syn_graphs():
    #     run_im_compact(g,0.2)


if __name__ == '__main__':
    # run_im_all()
    run_im_compact_all()
