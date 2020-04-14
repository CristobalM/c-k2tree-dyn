#include <bitvector.h>
#include <circular_queue.h>
#include <vector.h>

#include "block-frontier/block_frontier.h"
#include "block.h"
#include "definitions.h"
#include "morton-code/morton_code.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-variable"

struct child_result {
  struct block *resulting_block;
  uint32_t resulting_node_idx;
  uint32_t resulting_relative_depth;
  int is_leaf_result;
  int exists;
};

struct node_subtree_info {
  uint32_t node_index;
  uint32_t node_relative_depth;
  uint32_t subtree_size;
};

struct point_search_result {
  struct child_result last_child_result_reached;
  uint32_t depth_reached;
  int point_exists;
  uint32_t treedepth;
};

struct insertion_location {
  uint32_t insertion_index;
  struct point_search_result parent_node;
  uint32_t remaining_depth;
};

uint32_t skip_table[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1,
                         0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 2, 0, 0, 1, 2,
                         0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 2, 0, 1, 1, 2,
                         0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 3, 0, 1, 2, 3};

static inline uint32_t get_subtree_skipping_qty(struct block *b,
                                                uint32_t node_idx,
                                                uint32_t child_idx) {
  uint32_t node_value;
  _SAFE_OP_K2(read_node(b->bt, node_idx, &node_value));
  return skip_table[4 * node_value + child_idx];
}

static inline int mark_subtree_size(struct sequential_scan_result *sc_result,
                                    uint32_t node_index,
                                    uint32_t node_relative_depth,
                                    uint32_t subtree_size) {
  _SAFE_OP_K2(set_element_at(sc_result->subtrees_count_map,
                             (char *)&subtree_size, node_index));
  _SAFE_OP_K2(set_element_at(sc_result->relative_depth_map,
                             (char *)&node_relative_depth, node_index));

  return SUCCESS_ECODE;
}

/* INTERNAL FUNCTIONS PROTOTYPES */
int sequential_scan_child(struct block *input_block, uint32_t input_node_idx,
                          uint32_t subtrees_to_skip,
                          uint32_t *frontier_traversal_idx,
                          uint32_t input_node_relative_depth,
                          struct queries_state *qs);

/**
  Posible return codes:
  SUCCESS_CODE: child was found and stored in output variables
  DOES_NOT_EXIST_CHILD_ERR: child doesn't exist and output variables don't hold
**/
int child(struct block *input_block, uint32_t input_node_idx,
          uint32_t requested_child_position, uint32_t input_node_relative_depth,
          struct child_result *result, struct queries_state *qs);

int find_point(struct block *input_block, struct queries_state *qs,
               struct point_search_result *psr);

int find_insertion_location(struct block *input_block, struct queries_state *qs,
                            struct insertion_location *result);

uint32_t get_previous_siblings_count(struct block *input_block,
                                     struct child_result *parent_node_result,
                                     uint32_t child_code);

int init_sequential_scan_result(struct sequential_scan_result *scr,
                                uint32_t tree_depth);

int clean_sequential_scan_result(struct sequential_scan_result *scr);
/* END INTERNAL FUNCTIONS  PROTOTYPES */

/* INTERNAL FUNCTIONS IMPLEMENTATIONS */

int child(struct block *input_block, uint32_t input_node_idx,
          uint32_t requested_child_position, uint32_t input_node_relative_depth,
          struct child_result *result, struct queries_state *qs) {
  uint32_t tree_depth = input_block->tree_depth;
  if (input_node_relative_depth + 1 == tree_depth) {
    /* Create leaf result */
    result->resulting_block = input_block;
    result->resulting_node_idx = input_node_idx;
    result->resulting_relative_depth = input_node_relative_depth;
    result->is_leaf_result = TRUE;
    return SUCCESS_ECODE;
  }

  reset_circular_queue(&qs->not_yet_traversed);
  reset_circular_queue(&qs->subtrees_count);

  int exists;
  CHECK_ERR(child_exists(input_block->bt, input_node_idx,
                         requested_child_position, &exists));
  if (!exists)
    return DOES_NOT_EXIST_CHILD_ERR;

  uint32_t frontier_traversal_idx = 0;
  int is_frontier;
  CHECK_ERR(frontier_check(input_block->bf, input_node_idx,
                           &frontier_traversal_idx, &is_frontier));

  if (is_frontier) {
    struct block *child_block;
    get_child_block(input_block->bf, frontier_traversal_idx, &child_block);
    return child(child_block, 0, requested_child_position, 0, result, qs);
  }

  uint32_t subtrees_to_skip = get_subtree_skipping_qty(
      input_block, input_node_idx, requested_child_position);
  CHECK_ERR(sequential_scan_child(input_block, input_node_idx, subtrees_to_skip,
                                  &frontier_traversal_idx,
                                  input_node_relative_depth, qs));

  result->resulting_block = input_block;
  result->resulting_node_idx = qs->sc_result.child_preorder + 1;
  result->resulting_relative_depth = input_node_relative_depth + 1;

  return SUCCESS_ECODE;
}

int sequential_scan_child(struct block *input_block, uint32_t input_node_idx,
                          uint32_t subtrees_to_skip,
                          uint32_t *frontier_traversal_idx,
                          uint32_t input_node_relative_depth,
                          struct queries_state *qs) {
  struct sequential_scan_result *result = &qs->sc_result;
  if (subtrees_to_skip == 0) {
    result->child_preorder = input_node_idx;
    result->node_relative_depth = input_node_relative_depth;
    result->subtrees_count_map = NULL;
    result->relative_depth_map = NULL;
    return SUCCESS_ECODE;
  }

  _SAFE_OP_K2(
      push_circular_queue(&qs->not_yet_traversed, (char *)&subtrees_to_skip));

  struct node_subtree_info nsi;
  nsi.node_index = input_node_idx;
  nsi.node_relative_depth = input_node_relative_depth;
  nsi.subtree_size = 1;
  _SAFE_OP_K2(push_circular_queue(&qs->subtrees_count, (char *)&nsi));

  uint32_t current_node_index = input_node_idx;
  uint32_t depth = input_node_relative_depth;
  uint32_t real_depth = depth + input_block->block_depth;

  int is_empty;
  _SAFE_OP_K2(empty_circular_queue(&qs->not_yet_traversed, &is_empty));
  while (!is_empty) {
    current_node_index++;

    int is_frontier;
    CHECK_ERR(frontier_check(input_block->bf, input_node_idx,
                             frontier_traversal_idx, &is_frontier));

    int reaching_leaf = (real_depth == input_block->tree_depth - 1);

    uint32_t current_not_yet_traversed;
    _SAFE_OP_K2(back_circular_queue(&qs->not_yet_traversed,
                                    (char *)&current_not_yet_traversed));

    if (!reaching_leaf && !is_frontier) {
      depth++;
    }

    struct node_subtree_info nsi_w;
    nsi_w.node_index = input_node_idx;
    nsi_w.node_relative_depth = depth;
    nsi_w.subtree_size = 1;
    _SAFE_OP_K2(push_circular_queue(&qs->subtrees_count, (char *)&nsi_w));

    CHECK_ERR(frontier_check(input_block->bf, current_node_index,
                             frontier_traversal_idx, &is_frontier));
    real_depth = depth + input_block->block_depth;
    reaching_leaf = (real_depth == input_block->tree_depth - 1);

    uint32_t current_children_count;
    count_children(input_block->bt, current_node_index,
                   &current_children_count);

    if (current_children_count > 0 && !reaching_leaf && !is_frontier) {
      _SAFE_OP_K2(push_circular_queue(&qs->not_yet_traversed,
                                      (char *)&current_children_count));
    } else {
      uint32_t next_nyt;
      _SAFE_OP_K2(
          pop_back_circular_queue(&qs->not_yet_traversed, (char *)&next_nyt));
      next_nyt--;

      struct node_subtree_info nsi_rep;

      int is_info_queue_empty;
      _SAFE_OP_K2(
          empty_circular_queue(&qs->subtrees_count, &is_info_queue_empty));
      if (!is_info_queue_empty) {
        _SAFE_OP_K2(
            pop_back_circular_queue(&qs->subtrees_count, (char *)&nsi_rep));
        CHECK_ERR(mark_subtree_size(result, nsi_rep.node_index,
                                    nsi_rep.node_relative_depth,
                                    nsi_rep.subtree_size));
        _SAFE_OP_K2(
            empty_circular_queue(&qs->subtrees_count, &is_info_queue_empty));
        if (!is_info_queue_empty) {
          struct node_subtree_info *nsi_rep_within;
          back_reference_circular_queue(&qs->subtrees_count,
                                        (char **)&nsi_rep_within);
          nsi_rep_within->subtree_size += nsi_rep.subtree_size;
        }
      }
      _SAFE_OP_K2(empty_circular_queue(&qs->not_yet_traversed, &is_empty));
      while (next_nyt == 0 && !is_empty) {
        _SAFE_OP_K2(
            pop_back_circular_queue(&qs->not_yet_traversed, (char *)&next_nyt));
        next_nyt--;

        if (!is_info_queue_empty) {
          _SAFE_OP_K2(
              pop_back_circular_queue(&qs->subtrees_count, (char *)&nsi_rep));
          CHECK_ERR(mark_subtree_size(result, nsi_rep.node_index,
                                      nsi_rep.node_relative_depth,
                                      nsi_rep.subtree_size));
          _SAFE_OP_K2(
              empty_circular_queue(&qs->subtrees_count, &is_info_queue_empty));
          if (!is_info_queue_empty) {
            struct node_subtree_info *nsi_rep_within;
            _SAFE_OP_K2(back_reference_circular_queue(
                &qs->subtrees_count, (char **)&nsi_rep_within));
            nsi_rep_within->subtree_size += nsi_rep.subtree_size;
          }
        }

        if (depth > 0) {
          depth--;
        }
        // Last op
        _SAFE_OP_K2(empty_circular_queue(&qs->not_yet_traversed, &is_empty));
      }

      if (next_nyt > 0) {
        push_circular_queue(&qs->not_yet_traversed, (char *)&next_nyt);
      }
    }

    // Last op
    _SAFE_OP_K2(empty_circular_queue(&qs->not_yet_traversed, &is_empty));
  }
  int is_info_queue_empty;
  _SAFE_OP_K2(empty_circular_queue(&qs->subtrees_count, &is_info_queue_empty));
  if (!is_info_queue_empty) {
    struct node_subtree_info nsi_out_w;
    _SAFE_OP_K2(
        pop_back_circular_queue(&qs->subtrees_count, (char *)&nsi_out_w));
    CHECK_ERR(mark_subtree_size(result, nsi_out_w.node_index,
                                nsi_out_w.node_relative_depth,
                                nsi_out_w.subtree_size));

    _SAFE_OP_K2(
        empty_circular_queue(&qs->subtrees_count, &is_info_queue_empty));
    if (!is_info_queue_empty) {
      struct node_subtree_info *nsi_rep_within;
      _SAFE_OP_K2(back_reference_circular_queue(&qs->subtrees_count,
                                                (char **)&nsi_rep_within));
      nsi_rep_within->subtree_size += nsi_out_w.subtree_size;
    }
  }

  result->child_preorder = current_node_index;
  result->node_relative_depth = depth;

  return SUCCESS_ECODE;
}

int init_sequential_scan_result(struct sequential_scan_result *scr,
                                uint32_t tree_depth) {
  _SAFE_OP_K2(init_vector_with_capacity(scr->subtrees_count_map,
                                        sizeof(uint32_t), tree_depth));
  _SAFE_OP_K2(init_vector_with_capacity(scr->relative_depth_map,
                                        sizeof(uint32_t), tree_depth));
  return SUCCESS_ECODE;
}

int clean_sequential_scan_result(struct sequential_scan_result *scr) {
  _SAFE_OP_K2(free_vector(scr->subtrees_count_map));
  _SAFE_OP_K2(free_vector(scr->relative_depth_map));
  return SUCCESS_ECODE;
}

int find_point(struct block *input_block, struct queries_state *qs,
               struct point_search_result *psr) {
  struct child_result current_cr;
  current_cr.resulting_node_idx = 0;
  current_cr.resulting_block = input_block;
  current_cr.resulting_relative_depth = 0;
  current_cr.is_leaf_result = FALSE;

  uint32_t depth = input_block->block_depth;
  uint32_t relative_depth = 0;
  for (depth = input_block->block_depth; depth < input_block->tree_depth;
       depth++) {
    relative_depth = depth - input_block->block_depth;
    struct child_result prev_cr = current_cr;
    uint32_t current_mcode;
    _SAFE_OP_K2(get_code_at_morton_code(&qs->mc, depth, &current_mcode));
    _SAFE_OP_K2(child(input_block, current_cr.resulting_node_idx, current_mcode,
                      relative_depth, &current_cr, qs));

    psr->treedepth = input_block->tree_depth;

    if (current_cr.is_leaf_result) {
      uint32_t leaf_code;
      _SAFE_OP_K2(leaf_child_morton_code(&qs->mc, &leaf_code));

      int does_child_exists;
      _SAFE_OP_K2(child_exists(input_block->bt, current_cr.resulting_node_idx,
                               leaf_code, &does_child_exists));
      if (does_child_exists) {
        psr->last_child_result_reached = current_cr;
        psr->depth_reached = relative_depth + 1;
        psr->point_exists = TRUE;
      } else {
        psr->last_child_result_reached = prev_cr;
        psr->depth_reached = relative_depth;
        psr->point_exists = FALSE;
      }
      return SUCCESS_ECODE;
    }

    if (!current_cr.exists) {
      psr->last_child_result_reached = prev_cr;
      psr->depth_reached = relative_depth;
      psr->point_exists = FALSE;
      return SUCCESS_ECODE;
    }
  }

  // If the loop ends, then we have found a point
  psr->last_child_result_reached = current_cr;
  psr->depth_reached = relative_depth;
  psr->point_exists = TRUE;

  return SUCCESS_ECODE;
}

int find_insertion_location(struct block *input_block, struct queries_state *qs,
                            struct insertion_location *result) {
  struct point_search_result psr;
  CHECK_ERR(find_point(input_block, qs, &psr));

  struct block *reached_block = psr.last_child_result_reached.resulting_block;

  uint32_t frontier_traversal_idx = 0;
  uint32_t node_index = psr.last_child_result_reached.resulting_node_idx;

  result->parent_node = psr;

  if (psr.point_exists || psr.depth_reached == input_block->tree_depth - 1) {
    result->insertion_index = node_index;
    result->remaining_depth = 0;
    return SUCCESS_ECODE;
  }

  int is_frontier;
  CHECK_ERR(frontier_check(reached_block->bf, node_index,
                           &frontier_traversal_idx, &is_frontier));
  if (is_frontier) {
    return FRONTIER_NODE_WITHIN_FIND_INSERTION_LOC;
  }

  uint32_t child_code;
  CHECK_ERR(get_code_at_morton_code(&qs->mc, psr.depth_reached, &child_code));

  uint32_t to_be_skipped_subtrees = get_previous_siblings_count(
      reached_block, &psr.last_child_result_reached, child_code);

  if (to_be_skipped_subtrees == 0 && psr.depth_reached == 0) {
    if (reached_block->bt->nodes_count == 0) {
      result->insertion_index = 0;
      result->remaining_depth = input_block->tree_depth - psr.depth_reached;
    } else {
      result->insertion_index = 1;
      result->remaining_depth = input_block->tree_depth - psr.depth_reached - 1;
    }
    return SUCCESS_ECODE;
  }

  CHECK_ERR(sequential_scan_child(
      reached_block, node_index, to_be_skipped_subtrees,
      &frontier_traversal_idx, psr.depth_reached - reached_block->block_depth,
      qs));

  result->insertion_index = qs->sc_result.child_preorder + 1;
  result->remaining_depth = input_block->tree_depth - 1 - psr.depth_reached;

  return SUCCESS_ECODE;
}

uint32_t get_previous_siblings_count(struct block *input_block,
                                     struct child_result *parent_node_result,
                                     uint32_t child_code) {
  if (parent_node_result->resulting_node_idx >= input_block->bt->nodes_count) {
    return 0;
  }
  return get_subtree_skipping_qty(
      input_block, parent_node_result->resulting_node_idx, child_code);
}

/* END INTERNAL FUNCTIONS IMPLEMENTATIONS */

/* EXTERNAL FUNCTIONS */

int has_point(struct block *input_block, ulong col, ulong row,
              struct queries_state *qs, int *result) {

  CHECK_ERR(convert_coordinates_to_morton_code(
      col, row, input_block->tree_depth, &qs->mc));

  struct point_search_result psr;
  CHECK_ERR(find_point(input_block, qs, &psr));

  *result = psr.point_exists;

  return SUCCESS_ECODE;
}

int insert_point(struct block *input_block, ulong col, ulong row,
                 struct queries_state *qs) {

  CHECK_ERR(convert_coordinates_to_morton_code(
      col, row, input_block->tree_depth, &qs->mc));
  struct insertion_location il;
  CHECK_ERR(find_insertion_location(input_block, qs, &il));

  return NOT_IMPLEMENTED;
}

#pragma clang diagnostic pop
