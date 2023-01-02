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
extern "C" {
#include <block.h>
#include <queries_state.h>
}

#include "fisher_yates.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

std::mt19937 rng(123321);

void random_benchmark_by_depth(uint32_t treedepth, uint32_t points_count);
void random_benchmark_by_depth_dense(uint32_t treedepth, uint32_t points_count);

int main(void) {

  for (uint64_t i = 2; i < 20; i++) {
    random_benchmark_by_depth(i, 1 << i);
  }
  random_benchmark_by_depth(24, 1 << 20);
  random_benchmark_by_depth(28, 1 << 20);
  random_benchmark_by_depth(30, 1 << 20);

  return 0;
}

void random_benchmark_by_depth(uint32_t treedepth, uint32_t points_count) {
  uint64_t side = 1 << treedepth;
  struct block *root_block = create_block();

  struct queries_state qs;
  init_queries_state(&qs, treedepth, MAX_NODES_IN_BLOCK, root_block);

  auto random_seq_1 = fisher_yates(points_count, side);
  auto random_seq_2 = fisher_yates(points_count, side);

  std::cout << "-------------------\n";
  std::cout << "Started random_benchmark_by_depth with treedepth = "
            << treedepth << " and points_count = " << points_count << std::endl;
  auto start = std::chrono::high_resolution_clock::now();

  int point_exists;
  for (size_t i = 0; i < points_count; i++) {
    insert_point(root_block, random_seq_1[i], random_seq_2[i], &qs,
                 &point_exists);
  }

  auto stop = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

  std::cout << "\n\nInsertion results\n";
  std::cout << "Total Time in Microseconds: " << duration.count() << std::endl;
  std::cout << "Average Time inserting one point in Microseconds: "
            << duration.count() / points_count << std::endl;

#ifdef DEBUG_STATS
  std::cout << "(Times altered by measurement)" << std::endl;
  std::cout << "Time spent on sequential scan in total "
            << qs.dstats.time_on_sequential_scan << " microseconds"
            << std::endl;
  std::cout << "Time spent on sequential scan in average "
            << qs.dstats.time_on_sequential_scan / points_count
            << " microseconds" << std::endl;
  std::cout << "Time spent on frontier_check in total "
            << qs.dstats.time_on_frontier_check << " microseconds" << std::endl;
  std::cout << "Time spent on frontier_check in average "
            << qs.dstats.time_on_frontier_check / points_count
            << " microseconds" << std::endl;
  std::cout << "Split count: " << qs.dstats.split_count << std::endl;
  qs.dstats.time_on_sequential_scan = 0;
  qs.dstats.time_on_frontier_check = 0;
  qs.dstats.split_count = 0;
#endif

  start = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < points_count; i++) {
    int has_point_result;
    has_point(root_block, random_seq_1[i], random_seq_2[i], &qs,
              &has_point_result);
    if (!has_point_result) {
      std::cerr << "Point " << i << " not found (" << random_seq_1[i] << ", "
                << random_seq_2[i] << ")" << std::endl;
      exit(1);
    }
  }
  stop = std::chrono::high_resolution_clock::now();

  duration =
      std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

  std::cout << "\n\nSuccesful Querying Results\n";
  std::cout << "Total Time in Microseconds: " << duration.count() << std::endl;
  std::cout << "Average Time querying one point in Microseconds: "
            << duration.count() / points_count << std::endl;
#ifdef DEBUG_STATS
  std::cout << "(Times altered by measurement)" << std::endl;
  std::cout << "Time spent on sequential scan in total "
            << qs.dstats.time_on_sequential_scan << " microseconds"
            << std::endl;
  std::cout << "Time spent on sequential scan in average "
            << qs.dstats.time_on_sequential_scan / points_count
            << " microseconds" << std::endl;
  std::cout << "Time spent on frontier_check in total "
            << qs.dstats.time_on_frontier_check << " microseconds" << std::endl;
  std::cout << "Time spent on frontier_check in average "
            << qs.dstats.time_on_frontier_check / points_count
            << " microseconds" << std::endl;
  std::cout << "Split count: " << qs.dstats.split_count << std::endl;
  qs.dstats.time_on_sequential_scan = 0;
  qs.dstats.time_on_frontier_check = 0;
  qs.dstats.split_count = 0;
#endif

  std::cout << "-------------------\n\n\n" << std::endl;

  free_rec_block(root_block);
  finish_queries_state(&qs);
}

void random_benchmark_by_depth_dense(uint32_t treedepth,
                                     uint32_t points_count) {
  uint64_t side = 1 << treedepth;
  struct block *root_block = create_block();

  struct queries_state qs;
  init_queries_state(&qs, treedepth, MAX_NODES_IN_BLOCK, root_block);

  auto random_seq_1 = fisher_yates(points_count, side);
  auto random_seq_2 = fisher_yates(points_count, side);

  std::cout << "-------------------\n";
  std::cout << "Started random_benchmark_by_depth_dense with treedepth = "
            << treedepth << " and points_count = " << points_count << std::endl;
  auto start = std::chrono::high_resolution_clock::now();

  int point_exists;

  for (size_t i = 0; i < points_count; i++) {
    for (size_t j = 0; j < points_count; j++) {
      insert_point(root_block, random_seq_1[i], random_seq_2[j], &qs,
                   &point_exists);
    }
  }

  auto stop = std::chrono::high_resolution_clock::now();

  auto duration =
      std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

  std::cout << "\n\nInsertion results\n";
  std::cout << "Total Time in Microseconds: " << duration.count() << std::endl;
  std::cout << "Average Time inserting one point in Microseconds: "
            << duration.count() / points_count << std::endl;

#ifdef DEBUG_STATS
  std::cout << "(Times altered by measurement)" << std::endl;
  std::cout << "Time spent on sequential scan in total "
            << qs.dstats.time_on_sequential_scan << " microseconds"
            << std::endl;
  std::cout << "Time spent on sequential scan in average "
            << qs.dstats.time_on_sequential_scan / points_count
            << " microseconds" << std::endl;
  std::cout << "Time spent on frontier_check in total "
            << qs.dstats.time_on_frontier_check << " microseconds" << std::endl;
  std::cout << "Time spent on frontier_check in average "
            << qs.dstats.time_on_frontier_check / points_count
            << " microseconds" << std::endl;
  std::cout << "Split count: " << qs.dstats.split_count << std::endl;
  qs.dstats.time_on_sequential_scan = 0;
  qs.dstats.time_on_frontier_check = 0;
  qs.dstats.split_count = 0;
#endif

  start = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < points_count; i++) {
    for (size_t j = 0; j < points_count; j++) {
      int has_point_result;
      has_point(root_block, random_seq_1[i], random_seq_2[j], &qs,
                &has_point_result);
      if (!has_point_result) {
        std::cerr << "Point " << i << " not found (" << random_seq_1[i] << ", "
                  << random_seq_2[j] << ")" << std::endl;
        exit(1);
      }
    }
  }
  stop = std::chrono::high_resolution_clock::now();

  duration =
      std::chrono::duration_cast<std::chrono::microseconds>(stop - start);

  std::cout << "\n\nSuccesful Querying Results\n";
  std::cout << "Total Time in Microseconds: " << duration.count() << std::endl;
  std::cout << "Average Time querying one point in Microseconds: "
            << duration.count() / points_count << std::endl;
#ifdef DEBUG_STATS
  std::cout << "(Times altered by measurement)" << std::endl;
  std::cout << "Time spent on sequential scan in total "
            << qs.dstats.time_on_sequential_scan << " microseconds"
            << std::endl;
  std::cout << "Time spent on sequential scan in average "
            << qs.dstats.time_on_sequential_scan / points_count
            << " microseconds" << std::endl;
  std::cout << "Time spent on frontier_check in total "
            << qs.dstats.time_on_frontier_check << " microseconds" << std::endl;
  std::cout << "Time spent on frontier_check in average "
            << qs.dstats.time_on_frontier_check / points_count
            << " microseconds" << std::endl;
  std::cout << "Split count: " << qs.dstats.split_count << std::endl;
  qs.dstats.time_on_sequential_scan = 0;
  qs.dstats.time_on_frontier_check = 0;
  qs.dstats.split_count = 0;
#endif

  std::cout << "-------------------\n\n\n" << std::endl;

  free_rec_block(root_block);
  finish_queries_state(&qs);
}