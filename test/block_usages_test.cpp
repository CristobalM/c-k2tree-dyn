//
// Created by cristobal on 29-04-20.
//

extern "C" {
#include <block.h>
#include <block_topology.h>
#include <block_frontier.h>
#include <queries_state.h>
}

#include <iostream>
#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

bool comparePair(struct pair2dl a, struct pair2dl b){
  return a.col < b.col || (a.col == b.col && a.row < b.row);
}

void printVec(std::vector<struct pair2dl> &v){
  for(int i = 0; i < v.size(); i++){
    std::cout << v[i].col << ", " << v[i].row << "; ";
  }
  std::cout << std::endl;
}

TEST(usages, naive_scan_1){
  int err_code;
  uint32_t treedepth = 5;
  struct block *root_block = create_block(treedepth);

  struct queries_state qs;
  init_queries_state(&qs, treedepth);


  std::vector<struct pair2dl> init_elements = {{0, 0},  {3, 3},   {15, 3},
                                  {3, 15}, {30, 31}, {31, 8}};

  int qty = init_elements.size();
  for (int i = 0; i < qty; i++) {
    insert_point(root_block, init_elements[i].col, init_elements[i].row, &qs);
  }

  struct vector result;
  init_vector_with_capacity(&result, sizeof(struct pair2dl), qty);

  err_code = naive_scan_points(root_block, &qs, &result);
  if (err_code) {
    exit(err_code);
  }

  std::vector<struct pair2dl> store_points;
  for (int i = 0; i < result.nof_items; i++) {
    struct pair2dl *current;
    get_element_at(&result, i, (char **)&current);
    store_points.push_back(*current);
    // ASSERT_EQ(array_pairs[i].col, current->col) << "COL FAILED AT i = " << i;
    // ASSERT_EQ(array_pairs[i].row, current->row) << "ROW FAILED AT i = " << i;
  }

  std::sort(init_elements.begin(), init_elements.end(), comparePair);
  std::sort(store_points.begin(), store_points.end(), comparePair);

  printVec(init_elements);
  printVec(store_points);

  ASSERT_EQ(init_elements.size(), store_points.size()) << "DIFFERENT SIZE";

  for(int i = 0; i < init_elements.size(); i++){
    ASSERT_EQ(init_elements[i].col, store_points[i].col) << "(col) DIFFERENT AT i = " << i;
    ASSERT_EQ(init_elements[i].row, store_points[i].row) << "(row) DIFFERENT AT i = " << i;
  }

  free_vector(&result);

  err_code = free_rec_block(root_block);
  if (err_code)
    printf("There was an error with free_rec_block: %d\n", err_code);
  err_code = finish_queries_state(&qs);
  if (err_code)
    printf("There was an error with finish_queries_state: %d\n", err_code);

  if (!err_code) {
    printf("No errors while freeing data\n");
  }
}


bool has_pair(struct vector * v, ulong col, ulong row){
  for(int i = 0; i < v->nof_items; i++){
    struct pair2dl *pair;
    get_element_at(v, i, (char **)&pair);
    if(col == pair->col && row == pair->row){
      return true;
    }
  }
  return false;
}


TEST(usages, report_column_test_1){
  int err_code;
  uint32_t treedepth = 5;
  struct block *root_block = create_block(treedepth);

  struct queries_state qs;
  init_queries_state(&qs, treedepth);


  std::vector<struct pair2dl> init_elements = {{0, 0},  {3, 3},   {15, 3},
                                  {3, 15}, {30, 31}, {31, 8}, {3, 30}, {3, 2}, {3,29}};

  int qty = init_elements.size();
  for (int i = 0; i < qty; i++) {
    insert_point(root_block, init_elements[i].col, init_elements[i].row, &qs);
  }


  struct vector result;
  init_vector_with_capacity(&result, sizeof(struct pair2dl), 10);

  err_code = report_column(root_block, 3, &qs, &result);
  if (err_code) {
    exit(err_code);
  }

  ASSERT_TRUE(has_pair(&result, 3, 3)) << "Cant find pair 3,3";
  ASSERT_TRUE(has_pair(&result, 3, 15)) << "Cant find pair 3,15";
  ASSERT_TRUE(has_pair(&result, 3, 30)) << "Cant find pair 3,30";
  ASSERT_TRUE(has_pair(&result, 3, 2)) << "Cant find pair 3,2";
  ASSERT_TRUE(has_pair(&result, 3, 29)) << "Cant find pair 3,29";

  ASSERT_EQ(result.nof_items, 5) << "Does not have 5 elements";

}



TEST(usages, report_row_test_1){
  int err_code;
  uint32_t treedepth = 5;
  struct block *root_block = create_block(treedepth);

  struct queries_state qs;
  init_queries_state(&qs, treedepth);


  std::vector<struct pair2dl> init_elements = {{0, 0},  {3, 3},   {15, 3},
                                               {3, 15}, {30, 31}, {31, 8}, {3, 30}, {3, 2}, {3,29}, {8, 15}, {9, 15}, {15, 15}};

  int qty = init_elements.size();
  for (int i = 0; i < qty; i++) {
    insert_point(root_block, init_elements[i].col, init_elements[i].row, &qs);
  }


  struct vector result;
  init_vector_with_capacity(&result, sizeof(struct pair2dl), 10);

  err_code = report_row(root_block, 15, &qs, &result);
  if (err_code) {
    exit(err_code);
  }

  ASSERT_TRUE(has_pair(&result, 3, 15)) << "Cant find pair 3,15";
  ASSERT_TRUE(has_pair(&result, 8, 15)) << "Cant find pair 8,15";
  ASSERT_TRUE(has_pair(&result, 9, 15)) << "Cant find pair 9,15";
  ASSERT_TRUE(has_pair(&result, 15, 15)) << "Cant find pair 15,15";

  ASSERT_EQ(result.nof_items, 4) << "Does not have 4 elements";

}