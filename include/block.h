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
#ifndef _BLOCK_H_
#define _BLOCK_H_

#include <stdint.h>

#include "bitvector.h"
#include "block_frontier.h"
#include "block_topology.h"
#include "definitions.h"
#include "queries_state.h"
#include "vectors.h"

/* definitions to simplify reporting */
#define REPORT_COLUMN 0
#define REPORT_ROW 1

#define REPORT_FIRST_HALF(which_report, child_pos)                             \
  (((which_report) == REPORT_COLUMN && (child_pos) < 2) ||                     \
   ((which_report) == REPORT_ROW && (child_pos) % 2 == 0))

#define REPORT_SECOND_HALF(which_report, child_pos)                            \
  (((which_report) == REPORT_COLUMN && (child_pos) >= 2) ||                    \
   ((which_report) == REPORT_ROW && (child_pos) % 2 == 1))

#define REPORT_CONTINUE_CONDITION(current_col, half_length, which_report,      \
                                  child_pos)                                   \
  (((current_col) < (half_length) &&                                           \
    REPORT_FIRST_HALF(which_report, child_pos)) ||                             \
   ((current_col) >= (half_length) &&                                          \
    REPORT_SECOND_HALF(which_report, child_pos)))

struct block {
  NODES_BV_T *preorders;
  struct block *children_blocks;

  BVCTYPE *container;

  NODES_BV_T children;
  CONTAINER_SZ_T container_size;
  NODES_BV_T nodes_count;
};

struct k2tree_measurement {
  unsigned long total_bytes;
  unsigned long total_blocks;
  unsigned long bytes_topology;
};

struct child_result {
  struct block *resulting_block;
  uint32_t resulting_node_idx;
  TREE_DEPTH_T resulting_relative_depth;
  TREE_DEPTH_T block_depth;
  int is_leaf_result;
  int exists;

  /* Variables to handle frontier issues */
  int check_frontier; /* True if descended into a frontier node and couldn't
                       find the requested node there */

  int went_frontier; /* True if descended into a frontier node */

  struct block *previous_block;
  uint32_t previous_preorder;
  uint32_t previous_to_current_index;
  TREE_DEPTH_T previous_depth;
};

typedef void (*point_reporter_fun_t)(unsigned long, unsigned long, void *);

typedef void (*coord_reporter_fun_t)(unsigned long, void *);

int has_point(struct block *input_block, unsigned long col, unsigned long row,
              struct queries_state *qs, int *result);

int insert_point(struct block *input_block, unsigned long col,
                 unsigned long row, struct queries_state *qs,
                 int *already_exists);

int delete_point(struct block *input_block, unsigned long col,
                 unsigned long row, struct queries_state *qs,
                 int *already_not_exists);

int naive_scan_points(struct block *input_block, struct queries_state *qs,
                      struct vector_pair2dl_t *result);

int scan_points_interactively(struct block *input_block,
                              struct queries_state *qs,
                              point_reporter_fun_t point_reporter,
                              void *report_state);

int report_column(struct block *input_block, unsigned long col,
                  struct queries_state *qs, struct vector_pair2dl_t *result);

int report_row(struct block *input_block, unsigned long row,
               struct queries_state *qs, struct vector_pair2dl_t *result);

int report_column_interactively(struct block *input_block, unsigned long col,
                                struct queries_state *qs,
                                point_reporter_fun_t point_reporter,
                                void *report_state);

int report_row_interactively(struct block *input_block, unsigned long row,
                             struct queries_state *qs,
                             point_reporter_fun_t point_reporter,
                             void *report_state);

struct block *create_block(void);

int free_rec_block(struct block *input_block);

int free_block(struct block *input_block);

struct k2tree_measurement measure_tree_size(struct block *input_block);

int debug_validate_block(struct block *input_block);
int debug_validate_block_rec(struct block *input_block);

void debug_print_block(struct block *b);
void debug_print_block_rec(struct block *b);
typedef struct {
  struct block *input_block;
  struct child_result cr;
  TREE_DEPTH_T block_depth;
  uint32_t last_iteration;
  uint32_t frontier_traversal_idx;
} lazy_naive_state;

define_stack_of_type(lazy_naive_state)

    struct lazy_handler_naive_scan_t {
  struct queries_state *qs;
  struct lazy_naive_state_stack states_stack;
  pair2dl_t next_result;
  int has_next;

  struct block *tree_root;
};

typedef struct {
  struct child_result current_cr;
  uint64_t current_coord;
  uint32_t last_iteration;
  uint32_t frontier_traversal_idx;
} lazy_report_band_state_t;

define_stack_of_type(lazy_report_band_state_t)

    struct lazy_handler_report_band_t {
  struct queries_state *qs;
  struct lazy_report_band_state_t_stack stack;
  int which_report;
  uint64_t next_result;
  int has_next;
  uint64_t coord_to_report;
  struct block *tree_root;
};

int naive_scan_points_lazy_init(struct block *input_block,
                                struct queries_state *qs,
                                struct lazy_handler_naive_scan_t *lazy_handler);

int naive_scan_points_lazy_clean(
    struct lazy_handler_naive_scan_t *lazy_handler);

int naive_scan_points_lazy_next(struct lazy_handler_naive_scan_t *lazy_handler,
                                pair2dl_t *result);

int naive_scan_points_lazy_has_next(
    struct lazy_handler_naive_scan_t *lazy_handler, int *result);

int naive_scan_points_lazy_reset(
    struct lazy_handler_naive_scan_t *lazy_handler);

int report_row_lazy_init(struct lazy_handler_report_band_t *lazy_handler,
                         struct block *input_block, struct queries_state *qs,
                         uint64_t coord);
int report_column_lazy_init(struct lazy_handler_report_band_t *lazy_handler,
                            struct block *input_block, struct queries_state *qs,
                            uint64_t coord);

int report_band_lazy_clean(struct lazy_handler_report_band_t *lazy_handler);
int report_band_next(struct lazy_handler_report_band_t *lazy_handler,
                     uint64_t *result);
int report_band_reset(struct lazy_handler_report_band_t *lazy_handler);

int report_band_has_next(struct lazy_handler_report_band_t *lazy_handler,
                         int *result);

int clean_child_result(struct child_result *cresult);

#endif /* _BLOCK_H_ */
