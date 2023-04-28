### Influence-Maximization

This repository contains code for our paper "Fast and Space-Efficient Parallel Algorithms for Influence Maximization.".

### Requirements

- g++ or clang with C++17 features support (Tested with g++ 12.1.1 and clang 14.0.6) on Linux machines.
- We use <a href="https://github.com/cmuparlay/parlaylib">parlaylib</a> for fork-join parallelism and <a href="https://github.com/cmuparlay/PAM">PAM</a> for parallel BST. They are provided as submodules in our repository.

### Getting Started

#### Download

```
git clone --recurse-submodules git@github.com:ucrparlay/Influence-Maxmization.git
```

#### Build and Run

```
make -j
./IM <graph.bin> -k 100 -R 256 -w 0.1 -compact 1.0
```

### Reference

Letong Wang, Xiangyun Ding, Yan Gu and Yihan Sun. Fast and Space-Efficient Parallel Algorithms for Influence Maximization. In submission.
