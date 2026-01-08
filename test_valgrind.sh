#!/bin/bash

echo "=== Memory Leak Check ==="
valgrind --leak-check=full --show-leak-kinds=all ./output/BENCH_1


echo -e "\n=== Cache Performance Analysis ==="
valgrind --tool=cachegrind ./output/BENCH_1
cg_annotate cachegrind.out.* | head -50

echo -e "\n=== Callgrind Profiling ==="
valgrind --tool=callgrind ./output/BENCH_1
callgrind_annotate callgrind.out.* | head -50
