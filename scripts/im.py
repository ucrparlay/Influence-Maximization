from graphs import graphs
import subprocess
import os

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
GRAPH_DIR = f"{CURRENT_DIR}/../../data"

k = 100
R = 256
compact = 1.0

if __name__ == '__main__':
    subprocess.call(f'mkdir -p {CURRENT_DIR}/../logs', shell=True)
    IM = f'{CURRENT_DIR}/../IM'
    general_cascade = f'{CURRENT_DIR}/../general_cascade'
    if not os.path.exists(IM) or not os.path.exists(general_cascade):
        print('Please build by `make -j` first.')
        assert (0)
    for graph, url, w in graphs:
        graph_file = f'{GRAPH_DIR}/{graph}.bin'
        if not os.path.exists(graph_file):
            print(f'\nWarning: {graph} does not exists')
            continue
        seeds_file = f'{CURRENT_DIR}/../logs/{graph}.txt'

        print(f'\nRunning IM on {graph_file}')
        cmd = f'{IM} {graph_file} -k {k} -R {R} -w {w} -compact {compact} > {seeds_file}'
        subprocess.call(cmd, shell=True)
        print(f'Seeds saved to {seeds_file}')

        cmd = f'{general_cascade} {graph_file} {seeds_file} -w {w} -k {k} -i 1000'
        subprocess.call(cmd, shell=True)
