import os
import sys
import subprocess
import graph


IM_DIR = os.path.dirname(os.path.abspath(__file__)) + '/..'


def run_im(graph, w=0.1):
    print(f'running IM on {graph}')
    command = f'{IM_DIR}/IM {graph} -k 200 -R 200 -w {w} -celf'
    file_out = f'{IM_DIR}/scripts/im_results.txt'
    mem_out = f'{IM_DIR}/scripts/im_mem.txt'
    subprocess.call(
        f'/usr/bin/time -v numactl -i all {command} 1>> {file_out} 2>> {mem_out}', shell=True)


def run_im_all():
    for g in graph.get_small_graphs():
        run_im(g, 0.1)
    for g in graph.get_large_graphs():
        run_im(g, 0.1)
    for g in graph.get_knn_graphs():
        run_im(g, 0.2)
    for g in graph.get_grid_graphs():
        run_im(g, 0.2)


def run_im_compact(graph, w=0.1, compact=0.05):
    print(f'running IM_compact on {graph}')
    command = f'{IM_DIR}/IM {graph} -k 200 -R 200 -w {w} -compact {compact}'
    file_out = f'{IM_DIR}/scripts/im_compact_{compact}_results.txt'
    mem_out = f'{IM_DIR}/scripts/im_compact_{compact}_mem.txt'
    subprocess.call(
        f'/usr/bin/time -v numactl -i all {command} 1>> {file_out} 2>> {mem_out}', shell=True)


def run_im_compact_all():
    for compact in [0.05, 0.1, 0.2, 0.3, 0.5, 1.0]:
        for g in graph.get_small_graphs():
            run_im_compact(g, 0.1, compact)
        for g in graph.get_large_graphs():
            run_im_compact(g, 0.1, compact)
        for g in graph.get_knn_graphs():
            run_im_compact(g, 0.2, compact)
        for g in graph.get_grid_graphs():
            run_im_compact(g, 0.2, compact)


if __name__ == '__main__':
    # run_im('/data/lwang323/graph/bin/HepPh_sym.bin', 0.1)
    # run_im_compact('/data/lwang323/graph/bin/HepPh_sym.bin', 0.1, 1.0)
    run_im_all()
    run_im_compact_all()
