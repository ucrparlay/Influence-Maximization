## Influence-Maximization

This repository contains code for our paper "Fast and Space-Efficient Parallel Algorithms for Influence Maximization".

### Requirements

- gcc or clang with C++17 features support (Tested with gcc 12 and clang 14) on Linux machines.
- We use <a href="https://github.com/cmuparlay/parlaylib">parlaylib</a> for fork-join parallelism and <a href="https://github.com/cmuparlay/PAM">PAM</a> for parallel BST. They are provided as submodules in our repository.

### Getting Started

Code download:
```
git clone --recurse-submodules git@github.com:ucrparlay/Influence-Maxmization.git
```

We provide 5 small graphs as examples. They are located in `./data`. Run the following commands to get started quickly:
```
make -j
python3 scripts/im.py
```

#### Download More Graphs

```
python3 scripts/download.py
```

This command will download all graphs used in this paper to `./data` (except ClueWeb, which is too large and can be found at <a href="http://webdatacommons.org/hyperlinkgraph/">Web Data Commons</a>). You can also find more graphs at <a href="http://snap.stanford.edu/">Stanford Network Analysis Project</a> or this <a href="https://drive.google.com/drive/folders/1ZuhfaLmdL-EyOiWYqZGD1rOy_oSFRWe4">google drive</a>.

#### Run Influence Maximization

```
./IM [graph_file] -k [num_seeds] -R [num_sketches] -w [uic_weight] -compact [alpha]
```

#### Evaluate Seeds

The seeds file should contains the following line:
```
seeds: [seed_1] [seed_2] ... [seed_k]
```

Use the following command to evaluate the seeds with Monte-Carlo simulations:

```
./general_cascade [graph_file] [seeds_file] -k [num_seeds] -w [uic_weight] -i [num_MCsimulations]
```

### Reference

Letong Wang, Xiangyun Ding, Yan Gu and Yihan Sun. Fast and Space-Efficient Parallel Algorithms for Influence Maximization. In submission.
