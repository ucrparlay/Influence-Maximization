
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
    'twitter_sym.bin',
    'friendster_sym.bin',
    'sd_arc_sym.bin',
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
    'Cosmo50_5_sym.bin',
]


small_graph_dir = '/data/lwang323/graph/bin/'
large_graph_dir = '/data/graphs/bin/'


def get_small_graphs():
    return list(map(lambda g: small_graph_dir + g, small_graphs))


def get_large_graphs():
    return list(map(lambda g: large_graph_dir + g, large_graphs))


def get_grid_graphs():
    return list(map(lambda g: large_graph_dir + g, grid_graphs))


def get_knn_graphs():
    return list(map(lambda g: large_graph_dir + g, knn_graphs))


def get_all_graphs():
    return get_small_graphs() + get_large_graphs() + get_grid_graphs() + get_knn_graphs()


if __name__ == '__main__':
    print(get_all_graphs())
