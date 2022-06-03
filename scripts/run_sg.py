import os
import sys
import subprocess
import graph


IM_DIR = os.path.dirname(os.path.abspath(__file__)) + '/..'


def run_sg(graph):
    print(f'running static greedy on {graph}')
    pmc_command = f'{IM_DIR}/static_greedy/mymain {graph} 50 100 bsg'
    file_out = f'{IM_DIR}/scripts/sg_results.txt'
    mem_out = f'{IM_DIR}/scripts/sg_mem.txt'
    subprocess.call(
        f'/usr/bin/time -v numactl -i all {pmc_command} 1>> {file_out} 2>> {mem_out}', shell=True)


if __name__ == '__main__':
    graphs = graph.get_all_graphs()
    for g in graphs:
        run_sg(g)
    # run_sg('/data/lwang323/graph/bin/HepPh_sym.bin')
