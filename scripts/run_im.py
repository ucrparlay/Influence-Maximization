import os
import sys
import subprocess
import graph


IM_DIR = os.path.dirname(os.path.abspath(__file__)) + '/..'


def run_im(graph):
    print(f'running IM on {graph}')
    command = f'{IM_DIR}/IM {graph} -k 200 -R 200 -w 0.1 -compact'
    file_out = f'{IM_DIR}/scripts/im_results_new.txt'
    mem_out = f'{IM_DIR}/scripts/im_mem_new.txt'
    subprocess.call(
        f'/usr/bin/time -v numactl -i all {command} 1>> {file_out} 2>> {mem_out}', shell=True)


if __name__ == '__main__':
    # graphs = graph.get_all_graphs()
    # for g in graphs:
    #     run_im(g)
    run_im('/data/graphs/bin/soc-LiveJournal1_sym.bin')
