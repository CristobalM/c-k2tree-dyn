#ifndef _BLOCK_H_
#define _BLOCK_H_

#include <stdint.h>

#include "block-frontier/block_frontier.h"
#include "block-topology/block_topology.h"

#include "queries_state.h"

struct block {
  struct block_topology *bt;
  struct block_frontier *bf;
  uint32_t block_depth;
  uint32_t tree_depth;
  uint32_t max_node_count;
};

int has_point(struct block *input_block, ulong col, ulong row,
              struct queries_state *qs, int *result);
int insert_point(struct block *input_block, ulong col, ulong row,
                 struct queries_state *qs);

struct block *create_block(uint32_t tree_depth);
int free_block(struct block *input_block);

#endif /* _BLOCK_H_ */
