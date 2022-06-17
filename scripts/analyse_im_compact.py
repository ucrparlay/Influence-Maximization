import os
import re


def reorder(a):
    assert len(a) == 21
    b = a[-5:]
    a = a[:-5]
    c = a[-4:]
    a = a[:-4] + b + c
    return a


def analyse_im():
    f1 = open(f'im_results.txt', 'r')
    f2 = open(f'im_mem.txt', 'r')
    res = f1.read()
    mem = f2.read()
    f1.close()
    f2.close()

    init_time = re.findall('init_sketches:.*', res)
    assert len(init_time) == 21
    init_time = list(map(lambda x: float(x.split(' ')[-1]), init_time))
    init_time = reorder(init_time)

    seed_time = re.findall('select time:.*', res)
    assert len(seed_time) == 21
    seed_time = list(map(lambda x: float(x.split(' ')[-1]), seed_time))
    seed_time = reorder(seed_time)

    total_time = []
    for i in range(21):
        total_time.append(init_time[i] + seed_time[i])

    memory_kb = re.findall('Maximum resident set size.*', mem)
    assert len(memory_kb) == 21
    memory_kb = list(map(lambda x: int(x.split(' ')[-1]), memory_kb))
    memory_kb = reorder(memory_kb)

    memory_gb = list(map(lambda x: x / 1000000.0, memory_kb))

    return {
        'init_time': init_time,
        'seed_time': seed_time,
        'total_time': total_time,
        'memory_kb': memory_kb,
        'memory_gb': memory_gb,
    }


def analyse_compact(compact):
    f1 = open(f'im_compact_{compact}_results.txt', 'r')
    f2 = open(f'im_compact_{compact}_mem.txt', 'r')
    res = f1.read()
    mem = f2.read()
    f1.close()
    f2.close()

    init_time = re.findall('init_sketches time:.*', res)
    assert len(init_time) == 21
    init_time = list(map(lambda x: float(x.split(' ')[-1]), init_time))
    init_time = reorder(init_time)

    seed_time = re.findall('select_seeds time:.*', res)
    assert len(seed_time) == 21
    seed_time = list(map(lambda x: float(x.split(' ')[-1]), seed_time))
    seed_time = reorder(seed_time)

    total_time = []
    for i in range(21):
        total_time.append(init_time[i] + seed_time[i])

    memory_kb = re.findall('Maximum resident set size.*', mem)
    assert len(memory_kb) == 21
    memory_kb = list(map(lambda x: int(x.split(' ')[-1]), memory_kb))
    memory_kb = reorder(memory_kb)

    memory_gb = list(map(lambda x: x / 1000000.0, memory_kb))

    return {
        'init_time': init_time,
        'seed_time': seed_time,
        'total_time': total_time,
        'memory_kb': memory_kb,
        'memory_gb': memory_gb,
    }


def draw_figure(data, file):
    print(f'Generating {file}')
    import matplotlib.pyplot as plt
    fig = plt.figure(figsize=(12, 8))
    plt.title(file)
    xs = []
    ys = []
    for k, v in data.items():
        if type(k) == float:
            xs.append(k)
            ys.append(v)
    plt.plot(xs, ys, marker='o')
    plt.hlines(data['im'], 0.0, 1.0, colors = "r", linestyles = "dashed")
    
    bottom, _ = plt.ylim()
    if bottom < 0:
        plt.ylim(bottom = 0)
    plt.xticks([0.05, 0.1, 0.2, 0.3, 0.5, 1.0])
    yticks = list(plt.yticks()[0])
    yticks.append(data['im'])
    plt.yticks(yticks)
    plt.savefig(file)
    plt.close(fig)


def main():
    data = {}
    data['im'] = analyse_im()
    for compact in [0.05, 0.1, 0.2, 0.3, 0.5, 1.0]:
        data[compact] = analyse_compact(compact)
    print(data.keys())

    with open('excel.txt', 'w') as f:
        for i in range(21):
            line = []
            for p in data.values():
                for l in p.values():
                    line.append(str(l[i]))
            line.append('\n')
            f.write(' '.join(line))

    graphs = [
        'HepPh_sym',
        'Epinions_sym',
        'Slashdot_sym',
        'DBLP_sym',
        'Youtube_sym',
        'Orkut_sym',
        'LiveJournal_sym',
        'Twitter_sym',
        'Friendster_sym',
        'Sd_arc',
        'USA_sym',
        'GE_sym',
        'HT-5',
        'HH-5_sym',
        'Ch-5_sym',
        'GL-5_sym',
        'COS-5_sym',
        'SQR_sym',
        'SQR-S_sym',
        'REC_sym',
        'REC-S_sym',
    ]
    columns = [
        'init_time',
        'seed_time',
        'total_time',
        'memory_kb',
        'memory_gb',
    ]
    for i in range(len(graphs)):
        path = f'./figures/{str(i).zfill(2)}_{graphs[i]}'
        if not os.path.exists(path):
            os.makedirs(path)
        for j in range(len(columns)):
            file = path + '/' + columns[j] + '.jpg'
            d = {}
            for k, v in data.items():
                d[k] = v[columns[j]][i]
            draw_figure(d, file)


if __name__ == '__main__':
    main()
