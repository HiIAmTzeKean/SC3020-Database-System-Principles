#ifndef BP_TREE_H
#define BP_TREE_H

#include "storage/storage.h"
#include <optional>
#include <utility>
#include <vector>

const int KEY_SIZE = 4;
const int MAX_HEIGHT = 20;

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

    Node *m_current;
    int m_index;
    int m_vector_index;
    const BPlusTree *m_tree;
  };

  Iterator begin() const;
  Iterator search(float key) const;
  Iterator end() const;

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

  inline bool is_leaf() const { return this->m_is_leaf; };
  inline Node *next_node() const { return this->m_next; };

  size_t key_count() const;
  float key_at(int index) const;
  // Returns the index where the smallest value that is larger or equal to the
  // key is located. For internal nodes, this returns the index of the node
  // where the key can likely be found. For leaf nodes, this returns where the
  // key is.
  size_t search_key(float key) const;

  std::vector<RecordPointer> records_at(int index) const;
  size_t record_count_at(int index) const;
  size_t leaf_entry_count() const;

  Node *child_node_at(int index) const;
  size_t child_node_count() const;

private:
  Node(int degree, bool is_leaf);
  static Node create_empty_internal_node();

  std::optional<CreatedSibling> insert_leaf(float key, RecordPointer record);
  std::optional<CreatedSibling> insert_internal(float key,
                                                RecordPointer record);

  Node *split_leaf_child(float key, RecordPointer record);
  CreatedSibling split_internal_child(float key, Node *record);

  bool m_is_leaf = 0;
  int m_degree = 0;
  int m_size = 0;
  float *m_keys = nullptr;
  Node **m_node_values = nullptr;

  Node *m_next = nullptr;

  std::vector<std::vector<RecordPointer>> m_record_values;
};

#endif // BP_TREE_H
