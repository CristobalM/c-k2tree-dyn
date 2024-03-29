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
#ifndef _BLOCK_TOPOLOGY_H
#define _BLOCK_TOPOLOGY_H

#include <bitvector.h>
#include <stdint.h>

typedef uint16_t NODES_COUNT_T;

struct block;

int create_block_topology(struct block *bt);

int free_block_topology(struct block *bt);

int init_block_topology(struct block *bt, NODES_COUNT_T nodes_count);
int child_exists(struct block *bt, NODES_COUNT_T input_node_idx,
                 uint32_t requested_child_position, int *result);
int read_node(struct block *bt, NODES_COUNT_T node_idx, uint32_t *result);
int count_children(struct block *bt, NODES_COUNT_T node_idx, uint32_t *result);
int enlarge_block_size_to(struct block *bt, uint32_t new_block_size);
int shift_right_nodes_after(struct block *bt, NODES_COUNT_T node_index,
                            NODES_COUNT_T nodes_to_insert);
NODES_COUNT_T get_allocated_nodes(struct block *bt);
int mark_child_in_node(struct block *bt, NODES_COUNT_T node_index,
                       uint32_t leaf_child, int *was_marked_already);
int insert_node_at(struct block *bt, NODES_COUNT_T node_index, uint32_t code);
int extract_sub_bitvector(struct block *bt, uint32_t from, uint32_t to,
                          struct block *result);
int collapse_nodes(struct block *bt, uint32_t from, uint32_t to);

uint32_t get_nodes_count(struct block *bt);
int set_nodes_count(struct block *bt, uint32_t nodes_count);
uint32_t get_nodes_capacity(struct block *bt);

int copy_nodes_between_blocks(struct block *src, struct block *dst,
                              int src_start, int dst_start, int amount);
int copy_nodes_between_blocks_uarr(uint32_t *src, int src_sz, uint32_t *dst,
                                   int dst_sz, int src_start, int dst_start,
                                   int amount);

#endif /* _BLOCK_TOPOLOGY_H */
