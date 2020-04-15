#include "queries_state.h"
#include "definitions.h"

/* PRIVATE PROTOTYPES */
int init_sequential_scan_result(struct sequential_scan_result *scr,
                                uint32_t tree_depth);
int clean_sequential_scan_result(struct sequential_scan_result *scr);
/* END PRIVATE PROTOTYPES */

/* IMPLEMENTATION PUBLIC FUNCTIONS */
int init_queries_state(struct queries_state *qs, uint32_t tree_depth) {
  CHECK_ERR(init_morton_code(&qs->mc, tree_depth));
  return init_sequential_scan_result(&qs->sc_result, tree_depth);
}

int finish_queries_state(struct queries_state *qs) {
  CHECK_ERR(clean_morton_code(&qs->mc));
  return clean_sequential_scan_result(&qs->sc_result);
}
/* END IMPLEMENTATION PUBLIC FUNCTIONS */

/* PRIVATE FUNCTIONS IMPLEMENTATION */
int init_sequential_scan_result(struct sequential_scan_result *scr,
                                uint32_t tree_depth) {
  scr->subtrees_count_map = calloc(1, sizeof(struct vector));
  scr->relative_depth_map = calloc(1, sizeof(struct vector));
  _SAFE_OP_K2(init_vector_with_capacity(scr->subtrees_count_map,
                                        sizeof(uint32_t), tree_depth));
  _SAFE_OP_K2(init_vector_with_capacity(scr->relative_depth_map,
                                        sizeof(uint32_t), tree_depth));
  return SUCCESS_ECODE;
}

int clean_sequential_scan_result(struct sequential_scan_result *scr) {
  _SAFE_OP_K2(free_vector(scr->subtrees_count_map));
  _SAFE_OP_K2(free_vector(scr->relative_depth_map));
  free(scr->subtrees_count_map);
  free(scr->relative_depth_map);
  return SUCCESS_ECODE;
}
/* END PRIVATE FUNCTIONS IMPLEMENTATION */
