import os
import sys
import subprocess
import graph


IM_DIR = os.path.dirname(os.path.abspath(__file__)) + '/..'


def run_graph(graph, w=0.1):
    print(f'generating edgelist of {graph} with weight {w}')
    graph_name = (graph.split("/")[-1])[:-4]
    file_out = f'/data2/lwang323/edgelist/{graph_name}_{w}.txt'
    command = f'{IM_DIR}/graph {graph} {file_out} -w {w}'
    # print(f'numactl -i all {command}')
    subprocess.call(
        f'numactl -i all {command}', shell=True)


def run_all():
    for g in graph.get_small_graphs():
        run_graph(g, 0.1)
    for g in graph.get_large_graphs():
        run_graph(g, 0.1)
    for g in graph.get_knn_graphs():
        run_graph(g, 0.2)
    for g in graph.get_grid_graphs():
        run_graph(g, 0.2)

if __name__ == '__main__':
    run_all()