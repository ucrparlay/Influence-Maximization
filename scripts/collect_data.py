import os
import sys
import subprocess
import re
from graphs import graphs

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))

def collect_data(graph, file_in, key_words):
    g = graph
    f = open(file_in,'r')
    res = f.read()
    f.close()
    data_lines = re.findall(f"{key_words}.*", res)
    data = list(map(lambda x: eval(x.split(" ")[-1]), data_lines))
    return data

if __name__ == '__main__':
    data_map=dict()
    data_map["sketch"]=dict()
    data_map['select']=dict()
    data_map['total']=dict()
    data_map["mem"]=dict()
    for graph, url, w, GRAPH_DIR in graphs:
        graph_file = f'{GRAPH_DIR}/{graph}.bin'
        if not os.path.exists(graph_file):
            print(f'\nWarning: {graph} does not exists')
            continue
        seeds_file = f'{CURRENT_DIR}/../logs/{graph}.txt'
        mem_file = f'{CURRENT_DIR}/../logs/{graph}_mem.txt'
        data_map["sketch"][graph]=collect_data(graph, seeds_file, "average sketch construction time:")
        data_map["select"][graph]=collect_data(graph, seeds_file, "average seed selection time:")
        data_map["total"][graph]=collect_data(graph, seeds_file, "average total time:")
        data_map["mem"][graph]=collect_data(graph, mem_file, "Maximum resident set size ")
    
    for i in range(4):
        for graph, url, w, GRAPH_DIR in graphs:
            print(f"{graph}, {data_map['sketch'][graph][i]}, {data_map['select'][graph][i]}, {data_map['total'][graph][i]}, {data_map['mem'][graph][i]}")
    
        