#ifndef BP_TREE_H
#define BP_TREE_H

#include "node.h"
#include "storage/storage.h"

const int KEY_SIZE = 4;
const int MAX_HEIGHT = 20;

int ceil_div(int a, int b);
int floor_div(int a, int b);

class BPlusTree {
public:
  BPlusTree(Storage *storage, int degree);

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
  int get_degree() { return this->m_degree; };
  int get_height();
  std::vector<float> get_root_keys();
  int get_number_of_nodes();

  Storage *storage = nullptr;

private:
  int m_degree = 0;
  NodePointer m_root;
};

#endif // BP_TREE_H
