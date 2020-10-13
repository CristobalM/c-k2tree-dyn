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
#include "queries_state.h"
#include "definitions.h"

#include "memalloc.h"

/* PRIVATE PROTOTYPES */
int init_sequential_scan_result(struct sequential_scan_result *scr);
int clean_sequential_scan_result(struct sequential_scan_result *scr);
/* END PRIVATE PROTOTYPES */

/* IMPLEMENTATION PUBLIC FUNCTIONS */
int init_queries_state(struct queries_state *qs, uint32_t tree_depth) {
  CHECK_ERR(init_morton_code(&qs->mc, tree_depth));
  init_circular_queue(&qs->not_yet_traversed, 2 * tree_depth, sizeof(uint32_t));
  init_circular_queue(&qs->subtrees_count, 2 * tree_depth,
                      sizeof(struct node_subtree_info));
  qs->find_split_data = FALSE;
  return init_sequential_scan_result(&qs->sc_result);
}

int finish_queries_state(struct queries_state *qs) {
  _SAFE_OP_K2(clean_circular_queue(&qs->subtrees_count));
  _SAFE_OP_K2(clean_circular_queue(&qs->not_yet_traversed));
  CHECK_ERR(clean_morton_code(&qs->mc));
  return clean_sequential_scan_result(&qs->sc_result);
}
/* END IMPLEMENTATION PUBLIC FUNCTIONS */

/* PRIVATE FUNCTIONS IMPLEMENTATION */
int init_sequential_scan_result(struct sequential_scan_result *scr) {
  scr->subtrees_count_map = k2tree_alloc_vector();
  scr->relative_depth_map = k2tree_alloc_vector();
  _SAFE_OP_K2(init_vector_with_capacity(scr->subtrees_count_map,
                                        sizeof(uint32_t), MAX_NODES_IN_BLOCK));
  _SAFE_OP_K2(init_vector_with_capacity(scr->relative_depth_map,
                                        sizeof(uint32_t), MAX_NODES_IN_BLOCK));
  return SUCCESS_ECODE;
}

int clean_sequential_scan_result(struct sequential_scan_result *scr) {
  _SAFE_OP_K2(free_vector(scr->subtrees_count_map));
  _SAFE_OP_K2(free_vector(scr->relative_depth_map));

  k2tree_free_vector(scr->subtrees_count_map);
  k2tree_free_vector(scr->relative_depth_map);

  return SUCCESS_ECODE;
}
/* END PRIVATE FUNCTIONS IMPLEMENTATION */
