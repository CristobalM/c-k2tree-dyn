#include "block_topology.h"

#include <string.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) > (b) ? (b) : (a))

#define CEIL_OF_DIV(dividend, divisor)                                         \
  (((dividend) / (divisor)) + (((dividend) % (divisor)) == 0 ? 0 : 1))
#define BITS_SIZE(type) (sizeof(type) * 8)

uint32_t uint_bits = BITS_SIZE(uint32_t);

/* PRIVATE PROTOTYPES */

int resize_bv_to(struct bitvector **bv_ptr, uint32_t new_size);
int shift_bv_right_from(struct bitvector *bv, uint32_t from_location,
                        uint32_t to_location, uint32_t shift_amount);

/* END PRIVATE PROTOTYPES */

int init_block_topology(struct block_topology *bt, struct bitvector *bv,
                        uint32_t nodes_count) {
  bt->bv = bv;
  bt->nodes_count = nodes_count;
  return 0;
}

int child_exists(struct block_topology *bt, uint32_t input_node_idx,
                 uint32_t requested_child_position, int *result) {
  struct bitvector *bv = bt->bv;
  int bit_on;
  _SAFE_OP_K2(
      bit_read(bv, input_node_idx * 4 + requested_child_position, &bit_on));
  *result = bit_on;

  return SUCCESS_ECODE;
}

int read_node(struct block_topology *bt, uint32_t node_idx, uint32_t *result) {
  CHECK_ERR(bits_read(bt->bv, 4 * node_idx, 4 * (node_idx + 1) - 1, result));
  return SUCCESS_ECODE;
}

int count_children(struct block_topology *bt, uint32_t node_idx,
                   uint32_t *result) {
  if (node_idx >= bt->nodes_count) {
    return 0;
  }

  uint32_t node;
  CHECK_ERR(read_node(bt, node_idx, &node));

  *result = POP_COUNT(node);

  return SUCCESS_ECODE;
}

/*
  TODO: Benchmark and try to implement with preallocated memory
 */
int resize_bv_to(struct bitvector **bv_ptr, uint32_t new_size) {

  struct bitvector *bv = *bv_ptr;
  if (bv->size_in_bits == new_size) {
    return SUCCESS_ECODE;
  }

  uint32_t new_container_size = CEIL_OF_DIV(new_size, uint_bits);

  if (new_container_size == bv->container_size) {
    return SUCCESS_ECODE;
  }

  struct bitvector *new_bv = calloc(1, sizeof(struct bitvector));
  _SAFE_OP_K2(init_bitvector(new_bv, new_size));

  if (new_container_size > bv->container_size) {
    memcpy(new_bv->container, bv->container,
           bv->container_size * sizeof(uint32_t));
  } else {
    memcpy(new_bv->container, bv->container,
           new_container_size * sizeof(uint32_t));
  }

  _SAFE_OP_K2(clean_bitvector(bv));
  *bv_ptr = new_bv;

  return SUCCESS_ECODE;
}

int shift_bv_right_from(struct bitvector *bv, uint32_t from_location,
                        uint32_t to_location, uint32_t shift_amount) {
  if (shift_amount == 0) {
    return SUCCESS_ECODE;
  }

  uint32_t original_size = bv->size_in_bits;

  if (to_location + shift_amount > original_size) {
    CHECK_ERR(resize_bv_to(&bv, to_location + shift_amount));
  }

  uint32_t from = MAX(to_location - uint_bits, from_location);
  uint32_t to = MIN(from + uint_bits - 1, to_location - 1);

  uint32_t bits_to_shift = MAX((to_location - 1) - from_location + 1, 0);
  uint32_t blocks_to_shift_fully = bits_to_shift / uint_bits;
  uint32_t extra_bits_to_shift = bits_to_shift % uint_bits;

  uint32_t prev_from = from;

  for (uint32_t i = 0; i < blocks_to_shift_fully; i++) {
    uint32_t block_to_shift;
    _SAFE_OP_K2(bits_read(bv, from, to, &block_to_shift));
    _SAFE_OP_K2(
        bits_write(bv, from + shift_amount, to + shift_amount, block_to_shift));

    prev_from = from;
    from -= uint_bits;
    to = prev_from - 1;
  }

  // restore from
  from = prev_from;

  if (extra_bits_to_shift > 0) {
    if (blocks_to_shift_fully > 0) {
      to = from - 1;
      from = to - extra_bits_to_shift + 1;
    }
    if (from <= to) {
      uint32_t to_shift;
      _SAFE_OP_K2(bits_read(bv, from, to, &to_shift));
      _SAFE_OP_K2(
          bits_write(bv, from + shift_amount, to + shift_amount, to_shift));
    }
  }

  // clean up
  uint32_t remaining = shift_amount;
  for (uint32_t clean_from = from_location;
       clean_from < from_location + shift_amount; clean_from += uint_bits) {
    uint32_t cleaning_bits = MIN(remaining, uint_bits);
    _SAFE_OP_K2(
        bits_write(bv, from_location, from_location + cleaning_bits - 1, 0));
    remaining -= cleaning_bits;
  }

  return SUCCESS_ECODE;
}

int enlarge_block_size_to(struct block_topology *bt, uint32_t new_block_size) {
  struct bitvector **bv_ptr = &bt->bv;
  CHECK_ERR(resize_bv_to(bv_ptr, new_block_size * 4));
  bt->bv = *bv_ptr; // Check if this step is needed

  return SUCCESS_ECODE;
}

int shift_right_nodes_after(struct block_topology *bt, uint32_t node_index,
                            uint32_t nodes_to_insert) {
  uint32_t next_size = bt->nodes_count + nodes_to_insert;
  if (next_size > get_allocated_nodes(bt)) {
    CHECK_ERR(enlarge_block_size_to(bt, next_size));
  }

  CHECK_ERR(shift_bv_right_from(bt->bv, 4 * (node_index + 1),
                                4 * bt->nodes_count, nodes_to_insert));

  if (node_index + 1 < bt->nodes_count) {
    bt->nodes_count += nodes_to_insert;
  }

  return SUCCESS_ECODE;
}

uint32_t get_allocated_nodes(struct block_topology *bt) {
  uint32_t allocated_bits = bt->bv->container_size * sizeof(uint32_t) * 8;
  return allocated_bits / 4;
}

int mark_child_in_node(struct block_topology *bt, uint32_t node_index,
                       uint32_t leaf_child) {
  _SAFE_OP_K2(bit_set(bt->bv, 4 * node_index + leaf_child));
  return SUCCESS_ECODE;
}

uint32_t to_4_bits_table[] = {1 << 3, 1 << 2, 1 << 1, 1 << 0};

int insert_node_at(struct block_topology *bt, uint32_t node_index,
                   uint32_t code) {
  // Enlarge if needed
  uint32_t start_position = 4 * node_index;
  if (start_position >= bt->bv->size_in_bits) {
    CHECK_ERR(enlarge_block_size_to(bt, node_index + 1));
  }

  uint32_t end_position = start_position + 3; // inclusive
  uint32_t four_bits_rep = to_4_bits_table[code];

  _SAFE_OP_K2(bits_write(bt->bv, start_position, end_position, four_bits_rep));

  if (node_index + 1 > bt->nodes_count) {
    bt->nodes_count = node_index + 1;
  }

  return SUCCESS_ECODE;
}

int extract_sub_bitvector(struct block_topology *bt, uint32_t from, uint32_t to,
                          struct bitvector *result) {
  uint32_t new_size = to - from + 1;
  uint32_t blocks_to_copy = new_size / uint_bits;
  uint32_t extra_bits = new_size % uint_bits;

  for (uint32_t block_index = 0; block_index < blocks_to_copy; block_index++) {
    uint32_t block;
    _SAFE_OP_K2(bits_read(bt->bv, from + block_index * uint_bits,
                          from + (block_index + 1) * uint_bits - 1, &block));
    result->container[block_index] = block;
  }

  if (extra_bits > 0) {
    uint32_t last_bits_start_pos = from + blocks_to_copy * uint_bits;
    uint32_t last_bints_end_pos = last_bits_start_pos + extra_bits - 1;
    uint32_t last_bits;
    _SAFE_OP_K2(
        bits_read(bt->bv, last_bits_start_pos, last_bints_end_pos, &last_bits));
    last_bits = last_bits << (uint_bits - extra_bits);
    result->container[blocks_to_copy] = last_bits;
  }
  /* shrink parent bitvector */
  CHECK_ERR(collapse_nodes(bt, from, to));

  return SUCCESS_ECODE;
}

static inline int shift_left_from(struct bitvector *bv, uint32_t from,
                                  uint32_t shift_amount) {
  uint32_t to = bv->size_in_bits;
  uint32_t amount_to_shift = to - from + 1;
  uint32_t blocks_to_shift = amount_to_shift / uint_bits;
  uint32_t extra_bits_to_shift = amount_to_shift % uint_bits;

  for (uint32_t i = 0; i < blocks_to_shift; i++) {
    uint32_t read_start_pos = from + i * uint_bits;
    uint32_t read_end_pos = read_start_pos + uint_bits - 1;
    uint32_t block_to_write;
    _SAFE_OP_K2(bits_read(bv, read_start_pos, read_end_pos, &block_to_write));

    uint32_t write_start_pos = from - shift_amount + i * uint_bits;
    uint32_t write_end_pos = write_start_pos + uint_bits - 1;
    _SAFE_OP_K2(bits_write(bv, write_start_pos, write_end_pos, block_to_write));
    /* clean read block */
    _SAFE_OP_K2(bits_write(bv, read_start_pos, read_end_pos, 0));
  }

  if (extra_bits_to_shift > 0) {
    uint32_t read_start_pos = to - extra_bits_to_shift + 1;
    uint32_t read_end_pos = to;
    uint32_t last_part;
    _SAFE_OP_K2(bits_read(bv, read_start_pos, read_end_pos, &last_part));

    uint32_t write_start_pos =
        from - shift_amount + blocks_to_shift * uint_bits;
    uint32_t write_end_pos = write_start_pos + extra_bits_to_shift - 1;
    _SAFE_OP_K2(bits_write(bv, write_start_pos, write_end_pos, last_part));
    /* clean read block */
    if (read_start_pos < write_end_pos) {
      if (write_end_pos < read_end_pos) {
        _SAFE_OP_K2(bits_write(bv, write_end_pos + 1, read_end_pos, 0));
      }
    } else {
      _SAFE_OP_K2(bits_write(bv, read_start_pos, read_end_pos, 0));
    }
  }

  return SUCCESS_ECODE;
}

static inline int collapse_bits(struct block_topology *bt, uint32_t from,
                                uint32_t to) {
  struct bitvector *bv = bt->bv;
  uint32_t bits_diff = to - from;
  uint32_t blocks_to_collpase = bits_diff / uint_bits;
  uint32_t extra_bits = bits_diff % uint_bits;

  for (uint32_t i = 0; i < blocks_to_collpase; i++) {
    _SAFE_OP_K2(bits_write(bv, from + i * uint_bits,
                           from + (i + 1) * uint_bits - 1, 0));
  }
  _SAFE_OP_K2(bits_write(bv, to - extra_bits, to, 0));

  uint32_t delete_count = bits_diff + 1;

  CHECK_ERR(shift_left_from(bv, to + 1, delete_count));
  CHECK_ERR(resize_bv_to(&bt->bv, bv->size_in_bits - delete_count));

  return SUCCESS_ECODE;
}

int collapse_nodes(struct block_topology *bt, uint32_t from, uint32_t to) {
  CHECK_ERR(collapse_bits(bt, 4 * from, 4 * (to + 1) - 1));
  bt->nodes_count -= (to - from + 1);
  return SUCCESS_ECODE;
}
