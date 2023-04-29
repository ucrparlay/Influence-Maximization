### Influence-Maximization

This repository contains code for our paper "Fast and Space-Efficient Parallel Algorithms for Influence Maximization.".

### Requirements

- g++ or clang with C++17 features support (Tested with g++ 12 and clang 14) on Linux machines.
- We use <a href="https://github.com/cmuparlay/parlaylib">parlaylib</a> for fork-join parallelism and <a href="https://github.com/cmuparlay/PAM">PAM</a> for parallel BST. They are provided as submodules in our repository.

### Getting Started

#### Download Code

```
git clone --recurse-submodules git@github.com:ucrparlay/Influence-Maxmization.git
```

#### Download Graphs

The graphs used in our paper can be downloaded on <a href="https://drive.google.com/drive/folders/1ZuhfaLmdL-EyOiWYqZGD1rOy_oSFRWe4">google drive</a>.

Please use the graph names ending with `_sym.bin`.

#### Build and Run

```
make -j
./IM <graph> -k 100 -R 256 -w 0.1 -compact 1.0
```

### Reference

Letong Wang, Xiangyun Ding, Yan Gu and Yihan Sun. Fast and Space-Efficient Parallel Algorithms for Influence Maximization. In submission.
