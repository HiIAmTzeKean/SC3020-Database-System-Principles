#ifndef BP_TREE_H
#define BP_TREE_H

#include "storage/storage.h"
#include <utility>
#include <vector>

const int KEY_SIZE = 4;

int ceil_div(int a, int b);
int floor_div(int a, int b);

struct RecordPointer {
  float key;
  int offset;
  int block_id;
};

struct Node {
  Node(int degree, bool is_leaf);
  ~Node();

  std::pair<Node *, float> insert(float key, RecordPointer record);
  Node *split_leaf_child(float key, RecordPointer record);
  Node *split_internal_child(float key, Node *record);

  bool is_leaf = 0;
  int degree = 0;
  int size = 0;
  float *keys = nullptr;

  Node **node_values = nullptr;
  std::vector<std::vector<RecordPointer>> record_values;

  Node *next = nullptr;
};

class BPlusTree {
public:
  BPlusTree(int degree);
  ~BPlusTree();

  class Iterator {
  public:
    Iterator(Node *node, int index, float right_key, BPlusTree *tree);

    std::vector<Record> operator*() const;
    Iterator &operator++();
    bool operator!=(const Iterator &other) const;

  private:
    Node *current;
    int index;
    float right_key;
    BPlusTree *tree;
  };

  Iterator search_range_begin(float left_key, float right_key);
  Iterator search_range_end();
  std::pair<Iterator, Iterator> search_range_iter(float left_key,
                                                  float right_key);

  int get_index(float key, Node *node);
  Node *search_leaf_node(float key);
  std::vector<Record> search(float key);
  void insert(float key, RecordPointer value);
  void print();
  void print_node(Node *node, int level);
  int get_height();
  std::vector<float> get_root_keys();
  int get_number_of_nodes();
  void task_2();
  void task_3();

  int index_block_hit = 0;
  int data_block_hit = 0;
  Storage *storage = nullptr;

private:
  int degree = 0;
  Node *root = nullptr;
};

#endif // BP_TREE_H
