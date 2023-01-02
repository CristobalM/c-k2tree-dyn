/*
MIT License

Copyright (c) 2020 Cristobal Miranda T.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

#include "fisher_yates.hpp"

extern "C" {
#include "block.h"
}

std::mt19937 rng(123321);

struct BenchmarkResult {
  int treedepth;
  int node_count;
  uint64_t points_count;
  uint64_t size_bytes;
  uint64_t bytes_ptrs;
  uint64_t bytes_topologies;
  uint64_t total_microseconds_inserting;
  uint64_t inserted_points;
};

BenchmarkResult space_benchmark_random_insertion_by_depth_and_node_count(
    TREE_DEPTH_T treedepth, MAX_NODE_COUNT_T node_count,
    uint64_t points_count, std::vector<uint64_t> &cols,
    std::vector<uint64_t> &rows);

int main(void) {

  uint64_t points_count = 1 << 25;
  uint64_t treedepth = 30;
  uint64_t side = 1 << treedepth;
  uint64_t side_count =
      std::min((uint64_t)std::sqrt(points_count), side);

  auto random_seq_1 = fisher_yates(side_count, side);
  auto random_seq_2 = fisher_yates(side_count, side);
  std::vector<BenchmarkResult> results;
  std::cout << "size of block" << sizeof(struct block) << std::endl;
  std::cout << "started experiments" << std::endl;
  for (int node_count_base = 6; node_count_base <= 10; node_count_base++) {

    results.push_back(space_benchmark_random_insertion_by_depth_and_node_count(
        treedepth, 1 << node_count_base, points_count, random_seq_1,
        random_seq_2));
  }

  std::cout << "Tree depth,Node count,Points count,Total Bytes,Bytes "
               "Ptrs+Preorders,Bytes Topologies,Total Time(Microsecs), Avg "
               "Time(Microsecs)"
            << std::endl;

  for (auto bm : results) {
    std::cout << bm.treedepth << "," << bm.node_count << ","
              << bm.inserted_points << "," << bm.size_bytes << ","
              << bm.bytes_ptrs << "," << bm.bytes_topologies << ","
              << bm.total_microseconds_inserting << ","
              << (float)bm.total_microseconds_inserting /
                     (float)bm.inserted_points
              << std::endl;
  }

  return 0;
}

BenchmarkResult space_benchmark_random_insertion_by_depth_and_node_count(
    TREE_DEPTH_T treedepth, MAX_NODE_COUNT_T node_count,
    uint64_t points_count, std::vector<uint64_t> &cols,
    std::vector<uint64_t> &rows) {
  struct block *root_block = create_block();

  struct queries_state qs;
  init_queries_state(&qs, treedepth, node_count, root_block);

  auto start = std::chrono::high_resolution_clock::now();
  uint64_t inserted_points = 0;
  int point_exists;
  for (size_t i = 0; i < cols.size(); i++) {
    if (i * rows.size() > points_count)
      break;
    for (size_t j = 0; j < rows.size(); j++) {
      if (i * rows.size() + j > points_count)
        break;
      insert_point(root_block, cols[i], rows[j], &qs, &point_exists);
      inserted_points++;
    }
  }
  auto stop = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

  struct k2tree_measurement measurements = measure_tree_size(root_block);

  free_rec_block(root_block);
  finish_queries_state(&qs);

  return {treedepth,
          node_count,
          points_count,
          measurements.total_bytes,
          measurements.total_blocks *
              (sizeof(uint32_t) + sizeof(struct block *)),
          measurements.bytes_topology,
          (uint64_t)duration.count(),
          inserted_points};
}