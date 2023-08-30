from graphs import graphs
import subprocess
import os

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))

k = 100
R = 256
# compact = 1.0
compact = 0.1

if __name__ == '__main__':
    # subprocess.call(f'mkdir -p {CURRENT_DIR}/../our_log', shell=True)
    general_cascade = f'{CURRENT_DIR}/../general_cascade'
    folders = ["our_log", "infuser_log"]
    ids = [(0,2), (0, 1)]
    if  not os.path.exists(general_cascade):
        print('Please build by `make -j` first.')
        assert (0)
    for i in range(1,2):
        folder= folders[i]
        _id = ids[i]
        file_out = f"{CURRENT_DIR}/../{folder}/scores.txt"
        for graph, url, w, GRAPH_DIR in graphs:
            graph_file = f'{GRAPH_DIR}/{graph}.bin'
            if not os.path.exists(graph_file):
                print(f'\nWarning: {graph} does not exists')
                continue
            seeds_file = f'{CURRENT_DIR}/../{folder}/{graph}.txt'
            print(f'\nEvaluating on {graph_file}')
            
            # test uniform distribution
            if (w == 0.02):
                u1 = 0
                u2 = 0.1
            if (w == 0.2):
                u1 = 0.1
                u2= 0.3
            rounds = 20000
            if (graph in ['twitter_sym', 'friendster_sym', 'sd_arc_sym']):
                rounds = 2000
            cmd = f'{general_cascade} {graph_file} {seeds_file} -ua {u1} -ub {u2} -id {_id[0]} -k {k} -i {rounds}'
            print(f'Seeds evaluation to {seeds_file}')
            try:
                subprocess.call(f"{cmd} | tee -a {file_out}", shell=True)
            except:
                subprocess.call(f"echo '------------' >> {file_out}", shell=True)
                continue
           
            cmd = f'{general_cascade} {graph_file} {seeds_file} -WIC -k {k} -id {_id[1]} -i {rounds}'
            print(f'Seeds evaluation to {seeds_file}')
            try:
                subprocess.call(f"{cmd} | tee -a {file_out}", shell=True)
            except:
                subprocess.call(f"echo '------------' >> {file_out}", shell=True)
                continue
    
