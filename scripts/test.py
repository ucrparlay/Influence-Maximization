from graphs import graphs
import subprocess
import os

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))

k = 100
R = 256
# compact = 1.0
compact = 0.1

def test_edge_distribution():
    subprocess.call(f'mkdir -p {CURRENT_DIR}/../logs', shell=True)
    IM = f'{CURRENT_DIR}/../IM'
    general_cascade = f'{CURRENT_DIR}/../general_cascade'
    if not os.path.exists(IM) or not os.path.exists(general_cascade):
        print('Please build by `make -j` first.')
        assert (0)
    for graph, url, w, GRAPH_DIR in graphs:
        graph_file = f'{GRAPH_DIR}/{graph}.bin'
        if not os.path.exists(graph_file):
            print(f'\nWarning: {graph} does not exists')
            continue
        seeds_file = f'{CURRENT_DIR}/../logs/{graph}.txt'
        mem_file = f'{CURRENT_DIR}/../logs/{graph}_mem.txt'
        print(f'\nRunning IM on {graph_file}')
        
        # test uniform distribution
        if (w == 0.02):
            u1 = 0
            u2=0.1
        if (w == 0.2):
            u1 = 0.1
            u2 = 0.3
        for compact in [1, 0.1]:
            cmd = f'{IM} {graph_file} -k {k} -R {R} -ua {u1} -ub {u2} -compact {compact} -t 3 >> {seeds_file}'
            print(f'Seeds saved to {seeds_file}')
        # subprocess.call(cmd, shell=True)
            subprocess.call(
            f'/usr/bin/time -v numactl -i all {cmd} 2>> {mem_file}', shell=True)
        for compact in [1, 0.1]:
            cmd = f'{IM} {graph_file} -k {k} -R {R} -WIC -compact {compact} -t 3 >> {seeds_file}'
            print(f'Seeds saved to {seeds_file}')
            subprocess.call(
            f'/usr/bin/time -v numactl -i all {cmd} 2>> {mem_file}', shell=True)
        

        # cmd = f'{general_cascade} {graph_file} {seeds_file} -w {w} -k {k} -i 20000'
        # subprocess.call(cmd, shell=True)

def test_evaluations():
    IM = f'{CURRENT_DIR}/../IM'
    outfile=f'{CURRENT_DIR}/../our_log/evaluations.txt'
    for graph, url, w, GRAPH_DIR in graphs:
        graph_file = f'{GRAPH_DIR}/{graph}.bin'
        if not os.path.exists(graph_file):
            print(f'\nWarning: {graph} does not exists')
            continue
        print(f'\nRunning IM on {graph_file}')
        cmd = f'{IM} {graph_file} -k {k} -R {R} -UIC {w} -compact 1 -t 0 -Q | tee -a {outfile}'
        subprocess.call(cmd, shell=True)


# test_evaluations()
if __name__ == "__main__":
    general_cascade = f"{CURRENT_DIR}/../general_cascade"
    graph='sd_arc_sym'
    GRAPH_DIR="/ssd0/graphs/links"    
    cmd1 = f"{general_cascade} {GRAPH_DIR}/{graph}.bin sd_arc1_sym.txt -ua 0 -ub 0.1 -k {100} -i 2000" 
    cmd2 = f'{general_cascade} {GRAPH_DIR}/sd_arc_sym.bin sd_arc2_sym.txt -WIC -k {100} -i 2000'
    subprocess.call(cmd1, shell=True)
    subprocess.call(cmd2, shell=True)