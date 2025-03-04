#ifndef BP_TREE_H
#define BP_TREE_H

#include "storage/storage.h"
#include <optional>
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

class Node;
class BPlusTree {
public:
  BPlusTree(Storage *storage, int degree);
  ~BPlusTree();

  class Iterator {
  public:
    Iterator(const BPlusTree *tree, Node *node, int index);

    Record &operator*() const { return *record(); };
    Record *operator->() const { return record(); };
    Iterator &operator++();
    bool operator!=(const Iterator &other) const;

  private:
    Record *record() const;

    Node *current;
    int index;
    int vector_index;
    const BPlusTree *tree;
  };

  Iterator begin() const;
  Iterator search(float key) const;
  Iterator end() const;

  int get_index(float key, Node *node);
  Node *search_leaf_node(float key) const;
  void insert(float key, RecordPointer value);
  void print();
  void print_node(Node *node, int level);
  int get_height();
  std::vector<float> get_root_keys();
  int get_number_of_nodes();
  void task_2();

  Storage *storage = nullptr;

private:
  int degree = 0;
  Node *root = nullptr;
};

class Node {
public:
  // Create empty leaf node.
  Node(int degree) : Node(degree, true) {};
  // Create internal node.
  Node(int degree, Node *a, float key, Node *b);
  ~Node();

  struct CreatedSibling {
    Node *node;
    float key;
  };

  // Inserts the provided record into self. If it is not possible to fit in the
  // current node, the sibling node created will be returned.
  std::optional<CreatedSibling> insert(float key, RecordPointer record);

private:
  Node(int degree, bool is_leaf);
  static Node create_empty_internal_node();

  friend class BPlusTree;
  friend class BPlusTree::Iterator;
  std::optional<CreatedSibling> insert_leaf(float key, RecordPointer record);
  std::optional<CreatedSibling> insert_internal(float key,
                                                RecordPointer record);

  Node *split_leaf_child(float key, RecordPointer record);
  CreatedSibling split_internal_child(float key, Node *record);

  bool is_leaf = 0;
  int degree = 0;
  int size = 0;
  float *keys = nullptr;

  Node **node_values = nullptr;
  std::vector<std::vector<RecordPointer>> record_values;

  Node *next = nullptr;
};

#endif // BP_TREE_H
