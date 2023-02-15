import os
import sys
import subprocess
import graph


IM_DIR = os.path.dirname(os.path.abspath(__file__)) + '/..'


def run_pmc(graph):
    print(f'running pmc on {graph}')
    pmc_command = f'{IM_DIR}/pruned-monte-carlo/benchmark {graph} 200 200'
    file_out = f'{IM_DIR}/scripts/pmc_results.txt'
    mem_out = f'{IM_DIR}/scripts/pmc_mem.txt'
    subprocess.call(
        f'/usr/bin/time -v numactl -i all {pmc_command} 1>> {file_out} 2>> {mem_out}', shell=True)


if __name__ == '__main__':
    print(IM_DIR)
    run_pmc(f'/data/graphs/bin/HT_5_sym.bin')
