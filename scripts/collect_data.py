import os
import sys
import subprocess
import graph
import re

IM_DIR = os.path.dirname(os.path.abspath(__file__))

def collect_time(graph, k, folder):
    g = graph.split('/')[-1]
    file_in = f'{IM_DIR}/{folder}/{g[:-4]}.txt'
    offset_length = 5
    after_length=1
    f = open(file_in,'r')
    lines = f.readlines()
    f.close()
    n = len(lines)// (k+offset_length+after_length)
    total_time = list(map(lambda i: lines[k+offset_length-1+i*(k+offset_length+after_length)].split(' ')[2].replace('\n',''),range(n)))
    first_round_time = list(map(lambda i: lines[offset_length + i*(k+offset_length+after_length)].split(' ')[2].replace('\n',''), range(n)))
    return total_time, first_round_time

def collect_mem(graph,folder):
    g =graph.split('/')[-1]
    file_in=f'{IM_DIR}/{folder}/{g[:-4]}.txt'
    f_in = open(file_in,'r')
    mem = f_in.read()
    f_in.close()
    memory_kb= re.findall("Maximum resident set size (kbytes):*", mem)
    memory_kb = list(map(lambda x: eval(x.split(" ")[-1]), memory_kb))
    memory_gb = list(map(lambda x: x/1000000, memory_kb))
    return memory_gb

def collect_data_all(k, folder):
    file_out =  f'{IM_DIR}/R_002_1.txt'
    graphs = graph.get_all_graphs()
    first_round_time={}
    total_time={}
    memory={}
    for g in graphs:
        g_short = graph.graphs_map[g.split('/')[-1]]
        try:
            time_total, time_first = collect_time(g, k, folder)
            memory_gb = collect_mem(g, folder)
            first_round_time[g_short]=time_first
            total_time[g_short]=time_total
            memory[g_short]=memory_gb
        except:
            continue
    
    with open(file_out, 'a') as f:
        f.write("total running time\n")
        for g in total_time.keys():
            f.write(g + " ")
            print(total_time[g])
            f.write(" ".join(total_time[g]))
            f.write('\n')
        f.write("first round running time\n")
        for g in first_round_time.keys():
            f.write(g+" ")
            f.write(" ".join(first_round_time[g]))
            f.write("\n")
        f.write("memory usage\n")
        for g in memory.keys():
            line = []
            line.append(g)
            for t in total_time[g]:
                line.append(str(t))
            line.append("\n")
            f.write(" ".join(line))
if __name__ == '__main__':
    collect_data_all(100, "R_002_1")