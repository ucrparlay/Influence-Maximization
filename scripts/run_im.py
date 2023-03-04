import os
import sys
import subprocess
import graph
import analyse_seeds
import time


IM_DIR = os.path.dirname(os.path.abspath(__file__))



def run_im_compact(graph, folder, w, option):
    print(f'running IM_compact on {graph}')
    g = graph.split('/')[-1]
    R=256
    compact_list = [1, 0.5, 0.25, 0.1, 0.05]
    file_out = f'{IM_DIR}/{folder}/{g[:-4]}.txt'
    mem_out = f'{IM_DIR}/{folder}/{g[:-4]}_mem.txt'
    # compact_list = [0.25, 0.1,0.05]
    for c in compact_list:        
        start_time = time.time()
        command = f'{IM_DIR}/../IM {graph} -k 100 -R {R} -w {w} -compact {c} {option}'
        subprocess.call(
        f'/usr/bin/time -v numactl -i all {command} 1>> {file_out} 2>> {mem_out}', shell=True)
        if (time.time() - start_time) > 1200:
            break
    


def run_im_compact_all(folder, option):
    for g in graph.get_small_graphs():
        run_im_compact(g,folder ,0.02, option)
    for g in graph.get_large_graphs():
        run_im_compact(g, folder,0.02, option)
    for g in graph.get_road_graphs():
        run_im_compact(g, folder, 0.2, option)
    for g in graph.get_knn_graphs():
        run_im_compact(g, folder,0.2, option)
    for g in graph.get_grid_graphs():
        run_im_compact(g, folder, 0.2, option)

def run_infuser_scale(graph,  w, folder, option):
    g = graph.split('/')[-1]
    file_out = f'{IM_DIR}/{folder}/{g[:-4]}.txt'
    R=256
    c=1.0
    command = f'{IM_DIR}/../IM {graph} -k 100 -R {R} -w {w} -compact {c} {option}'
    n_threads = [192, 96, 48, 24, 12, 8, 4, 2, 1]
    cores = ['0-191', '0-95', '0-95:2', '0-95:4', '0-47:4', '0-31:4', '0-15:4', '0-7:4','0-3:4']
    for i in range(len(n_threads)):
        numa = "numactl -i all"
        n_thread = n_threads[i]
        core = cores[i]
        print(f'running IM compact on {graph} n_thread {n_thread} cores {core}')
        if (":4" in core):
            numa = ''
        subprocess.call(f'echo n_threads {n_thread} cores {core} R {R} >> {file_out}',shell=True)
        subprocess.call(
            f'PARLAY_NUM_THREADS={n_thread} taskset -c {core} {numa} {command} >> {file_out}', shell=True)

def run_infuser_scale_all(folder, option):
    small_graphs = ['Epinions1_sym.bin',
    'Slashdot_sym.bin',
    'DBLP_sym.bin',
    'Youtube_sym.bin']
    large_graphs = ['com-orkut_sym.bin',
    'soc-LiveJournal1_sym.bin']
    sparse_graphs=[
    'RoadUSA_sym.bin',
    'Household.lines_5_sym.bin',
    'CHEM_5_sym.bin']
    for g in small_graphs:
        graph_path = graph.small_graphs_dir+g 
        run_infuser_scale(graph_path,0.02,folder, option)
    for g in large_graphs:
        graph_path = graph.large_graphs_dir+g 
        run_infuser_scale(graph_path, 0.02, folder, option)
    for g in sparse_graphs:
        graph_path = graph.large_graphs_dir+g
        run_infuser_scale(graph_path, 0.2, folder, option)

if __name__ == '__main__':
    subprocess.call("export LD_PRELOAD=/usr/local/lib/libjemalloc.so",shell=True)
    # folder='scale'
    folders = {"scale_stdQ": "-Q",
    "scale_BST": "-PAM"}
    # for folder, option in folders.items():
    #     run_infuser_scale_all(folder, option)
    # run_infuser_scale_all('scale_BST_nomax', '-PAM')
    # folders = ["stdQ2", "BST2", "WinTree2"]
    # options = ['-Q', '-PAM', '']
    # folders = ["Breakdown/stdQ", "Breakdown/BST", "Breakdown/WinTree"]
    # for c in compact_list:
    # for i in range(3):
    #     folder=folders[i]
    #     analyse_seeds.collect_time_all(folder, folder)
    #     analyse_seeds.collect_memory_all(folder, folder)
    # folder = folders[2]
    run_infuser_scale_all('scale_WinTree_sort', '')
    graphs = ['Epinions1_sym.bin', 'Slashdot_sym.bin',
    'DBLP_sym.bin', 'Youtube_sym.bin','com-orkut_sym.bin',
    'soc-LiveJournal1_sym.bin', 'RoadUSA_sym.bin',
    'Household.lines_5_sym.bin','CHEM_5_sym.bin']
    # ]
    analyse_seeds.collect_time_all("scale_WinTree_sort","scale_WinTree_sort", graphs)
    # analyse_seeds.collect_time_all( "scale_stdQ","scale_stdQ",  graphs)
    # analyse_seeds.collect_memory_all(folder, folder)
        # run_im_compact_all(folders[i], options[i])
    # analyse_seeds.collect_evalues_all(folder, folder)
    # compact_list = [1, 0.5, 0.25, 0.1, 0.05]
    # for c in compact_list:
    #     print(f"compact rate {c}")
    #     for i in range(3):
    #         run_im_compact_all(folders[i], c, options[i])
    # folder = "R"
    # run_im_compact_all(folder, 1, "")
    # analyse_seeds.analyse_im_seeds_all(folder)
    # analyse_seeds.collect_influence_all(folder, "R.txt")

    