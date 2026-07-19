# Latency Benchmark


## Overview

Latency Benchmark is a C++ framework for evaluating and comparing the performance of latency-critical code.

The project includes a high-performance order book implementation where all core operations execute in O(1) time. To achieve consistently low latency, the implementation is carefully optimized with data structures and memory layouts designed to maximize CPU efficiency while avoiding runtime overhead.

Key features include:

* No dynamic memory allocation during hot path
* Cache-friendly data layout to improve locality
* Single-Producer Single-Consumer (SPSC) queues for efficient inter-thread communication
* Modern C++ language attributes and compiler optimizations
* Focus on predictable, low-latency performance

More features and benchmarks are coming soon.

## Architecture
![architecture diagram](./images/benchmark.png)


## Build requirements

- Compiler: GCC 15.2.0+
- OS: Linux, MacOS

## Build instructions

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```