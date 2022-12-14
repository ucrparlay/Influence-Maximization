import os
import sys
import subprocess
import run_im
import analyse_seeds


IM_DIR = os.path.dirname(os.path.abspath(__file__)) + '/..'

if __name__ == '__main__':
    run_im.run_im_compact_all()
    analyse_seeds.analyse_im_seeds_all(IM_DIR)
    analyse_seeds.analyse_im_seeds_results_all(IM_DIR)