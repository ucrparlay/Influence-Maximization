## Influence-Maximization

This repository contains code for our paper "Fast and Space-Efficient Parallel Algorithms for Influence Maximization".

### Requirements

- Multi-processor linux machine (tested on CentOS 8 and MacOS 13)
- gcc or clang with C++17 features support (tested with gcc 12 and clang 14)
- python3 (used to run scripts)
- We use <a href="https://github.com/cmuparlay/parlaylib">parlaylib</a> for fork-join parallelism and <a href="https://github.com/cmuparlay/PAM">PAM</a> for parallel BST. They are provided as submodules in our repository.

### Getting Started

Code download:
```
git clone --recurse-submodules git@github.com:ucrparlay/Influence-Maximization.git
```

We provide 5 small graphs as examples. They are located in `./data`. Run the following commands to get started quickly:
```
make -j
python3 scripts/im.py
```

#### Download All Graphs

```
python3 scripts/download.py
```

This command will download the graphs used in this paper to `./data`.

We comment out `twitter_sym`, `friendster_sym`, `sd_arc_sym` and `Cosmo50_5_sym`. If you want to download these graphs, you can uncomment them in `./scripts/graphs.py`, but may not download successful because of the band width limitation of dropbox. You can try to download them another day.

For ClueWeb, it is too large to fit in dropbox. You can find it at <a href="http://webdatacommons.org/hyperlinkgraph/">Web Data Commons</a>.

You can also download graphs manually from this link <a href="https://drive.google.com/drive/folders/1C86nDTo76aalBcmtgWWBLW6sOIhe1Btq?usp=share_link">google drive</a>.

We use the `.bin` binary graph format from [GBBS](https://github.com/ParAlg/gbbs).

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
