
small_graphs = [
    'HepPh_sym.bin',
    'Epinions1_sym.bin',
    'Slashdot_sym.bin',
    'DBLP_sym.bin',
    'Youtube_sym.bin',
]

large_graphs = [
    'com-orkut_sym.bin',
    'soc-LiveJournal1_sym.bin',
    # 'twitter_sym.bin',
    # 'friendster_sym.bin',
    # 'sd_arc_sym.bin',
    'RoadUSA_sym.bin',
    'Germany_sym.bin',
]

grid_graphs = [
    'grid_4000_4000_sym.bin',
    'grid_4000_4000_03_sym.bin',
    'grid_1000_10000_sym.bin',
    'grid_1000_10000_03_sym.bin',
]

knn_graphs = [
    'HT_5_sym.bin',
    'Household.lines_5_sym.bin',
    'CHEM_5_sym.bin',
    'GeoLifeNoScale_5_sym.bin',
    # 'Cosmo50_5_sym.bin',
]


small_graph_dir = '/data0/lwang323/graph/bin/'
large_graph_dir = '/data0/graphs/bin/'
knn_graph_dir = '/data0/graphs/knn/'
grid_graph_dir = '/data0/graphs/synthetic/'


def get_small_graphs():
    return list(map(lambda g: small_graph_dir + g, small_graphs))


def get_large_graphs():
    return list(map(lambda g: large_graph_dir + g, large_graphs))


def get_grid_graphs():
    return list(map(lambda g: grid_graph_dir + g, grid_graphs))


def get_knn_graphs():
    return list(map(lambda g: knn_graph_dir + g, knn_graphs))


def get_all_graphs():
    return get_small_graphs() + get_large_graphs() + get_grid_graphs() + get_knn_graphs()


def get_directed_graphs():
    return [
        '/data/lwang323/graph/bin/Epinions1.bin',
        '/data/lwang323/graph/bin/Slashdot.bin',
        # '/data/graphs/bin/soc-LiveJournal1.bin',
        # '/data/graphs/bin/twitter.bin',
        # '/data/graphs/bin/sd_arc.bin',
        '/data/graphs/bin/HT_5.bin',
        # '/data/graphs/bin/Household.lines_5.bin',
        # '/data/graphs/bin/CHEM_5.bin',
        # '/data/graphs/bin/GeoLifeNoScale_5.bin',
        # '/data/graphs/bin/Cosmo50_5.bin',
        # '/data/graphs/bin/grid_4000_4000.bin',
        # '/data/graphs/bin/grid_1000_10000.bin',
        # '/data/graphs/bin/grid_4000_4000_03.bin',
        # '/data/graphs/bin/grid_1000_10000_03.bin',
    ]


if __name__ == '__main__':
    print(get_all_graphs())
    print(get_directed_graphs())
