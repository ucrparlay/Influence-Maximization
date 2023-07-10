
# If you want to download other graphs, you can uncomment the corresponding line,
# but may not download successful because of the band width limitation of dropbox.
# You can try to download them another day.
import os
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
SMALL_GRAPH_DIR=f"{CURRENT_DIR}/../data"
OTHER_GRAPH_DIR="/data0/graphs/links"

graphs = [
    # Social Graphs
    ('Epinions1_sym', '', 0.02, SMALL_GRAPH_DIR),
    ('Slashdot_sym', '', 0.02, SMALL_GRAPH_DIR),
    ('DBLP_sym', '', 0.02, SMALL_GRAPH_DIR),
    ('Youtube_sym', '', 0.02, SMALL_GRAPH_DIR),
    ('com-orkut_sym', 'https://www.dropbox.com/s/2okcj8q7q1z3nkw/com-orkut_sym.bin?dl=0', 0.02, OTHER_GRAPH_DIR),
    ('soc-LiveJournal1_sym', 'https://www.dropbox.com/s/6pbryf0wpxgmj7c/soc-LiveJournal1_sym.bin?dl=0', 0.02, OTHER_GRAPH_DIR),
    # ('twitter_sym', 'https://www.dropbox.com/s/vtnfg6e75h0jama/twitter_sym.bin?dl=0', 0.02),
    # ('friendster_sym', 'https://www.dropbox.com/s/6xiwui88bacbt6o/friendster_sym.bin?dl=0', 0.02),

    # Web Graphs
    # ('sd_arc_sym', 'https://www.dropbox.com/s/xtkei44fh9v5v73/sd_arc_sym.bin?dl=0', 0.02),
    # ('ClueWeb_sym', '', 0.02),  # too large

    # Road Graphs
    # ('Germany_sym', 'https://www.dropbox.com/s/42s5gb1gjs36x3z/Germany_sym.bin?dl=0', 0.2),
    # ('RoadUSA_sym', 'https://www.dropbox.com/s/a6dhyw9ec0d3hlh/RoadUSA_sym.bin?dl=0', 0.2),

    # KNN Graphs
    # ('HT_5_sym', '', 0.2),
    ('Household.lines_5_sym', 'https://www.dropbox.com/s/281aia4r73owtjk/Household.lines_5_sym.bin?dl=0', 0.2, OTHER_GRAPH_DIR),
    ('CHEM_5_sym', 'https://www.dropbox.com/s/r0c1smelm4lllbz/CHEM_5_sym.bin?dl=0', 0.2, OTHER_GRAPH_DIR),
    # ('GeoLifeNoScale_5_sym', 'https://www.dropbox.com/s/32cb0d645i2qf30/GeoLifeNoScale_5_sym.bin?dl=0', 0.2),
    # ('Cosmo50_5_sym', 'https://www.dropbox.com/s/8oxqut1ff7l73ws/Cosmo50_5_sym.bin?dl=0', 0.2),
]
