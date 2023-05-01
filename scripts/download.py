from graphs import graphs
import subprocess
import os

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))

if __name__ == '__main__':
    for graph, url, w in graphs:
        print(f'Downloading {graph} from {url}')
        graph_file = f'{CURRENT_DIR}/../data/{graph}.bin'
        if not os.path.exists(graph_file):
            if len(url) == 0:
                print(f'Cannot download {graph}!')
                continue
            subprocess.call(f'wget -O {graph_file} {url}', shell=True)
            print(f'Successfully downloaded {graph}')
        else:
            print(f'Using pre-downloaded {graph}')
