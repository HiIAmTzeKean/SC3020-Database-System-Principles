#include "record.h"
#include <algorithm>
#include <stddef.h>

constexpr int BPlusTreeN = 8;

// ASSUMPTION: No duplicate keys will be inserted.
// See BPlusTree at bottom of file.

class BPlusNode {
public:
  struct Iterator;

  BPlusNode() : BPlusNode(true) {};
  BPlusNode(bool is_leaf_node) : is_leaf_node(is_leaf_node) {};
  ~BPlusNode();

  Iterator first() const;
  Iterator search(float key) const;
  Record *search_exact(float key) const;
  BPlusNode &insert(float key, Record *record);

  void print() const;

  bool is_leaf_node;

private:
  Iterator search_node(float key) const;
  size_t entry_count() const;
  void set_entry_count(size_t entry_count);

  union Entry {
    BPlusNode *node;
    Record *record;
  };

  BPlusNode(bool is_leaf_node, Entry entries[BPlusTreeN + 1])
      : is_leaf_node(is_leaf_node), m_entries() {
    std::copy(entries, entries + BPlusTreeN + 1, m_entries);
  };

  // Internal nodes.
  BPlusNode *node(size_t index) const;

  // Leaf nodes.
  BPlusNode *next_ptr() const;
  void set_next_ptr(BPlusNode *ptr);
  Record *record(size_t index) const;

  BPlusNode *m_parent{nullptr};
  size_t m_key_count{0};
  float m_keys[BPlusTreeN]{0};
  Entry m_entries[BPlusTreeN + 1]{0};
};

// This struct is intended to be used like a normal vector iterator. :)
// Example:
// for (auto it = btree.search(123); it->is_valid(); ++it) {}
struct BPlusNode::Iterator {
public:
  Iterator(const BPlusNode *node, size_t index)
      : m_node(node), m_index(index) {};

  bool is_valid() const;
  float current_key() const;
  Iterator &canonical();

  Record *record() const { return m_node->record(m_index); };
  Record &operator*() const { return *record(); };
  Record &operator->() const { return *record(); };
  Iterator &operator++();
  Iterator operator++(int) {
    Iterator old_iterator = *this;
    ++(*this);
    return old_iterator;
  };
  friend bool operator==(const Iterator &a, const Iterator &b) {
    return a.m_node == b.m_node && a.m_index == b.m_index;
  };
  friend bool operator!=(const Iterator &a, const Iterator &b) {
    return a.m_node != b.m_node || a.m_index != b.m_index;
  };

private:
  friend BPlusNode;

  const BPlusNode *m_node;
  size_t m_index;
};

class BPlusTree {
public:
  BPlusTree() { m_root = new BPlusNode(); };
  ~BPlusTree() { delete m_root; };

  using Node = BPlusNode;
  using Iterator = BPlusNode::Iterator;

  // first returns an iterator starting at the smallest value.
  Iterator first() const { return m_root->first(); };
  // search returns an iterator starting at the value larger or equal to key.
  // It may return an invalid iterator if the provided key is larger than any
  // known keys. If there are multiple keys with the same value, it will iterate
  // through all of them.
  Iterator search(float key) const { return m_root->search(key); };
  // search_exact returns the exact record, if it exists.
  // If multiple records exist with the key exists, it may not consistently
  // return the same record.
  Record *search_exact(float key) const { return m_root->search_exact(key); };
  // insert updates the key and returns the new root.
  void insert(float key, Record *record) {
    m_root = &m_root->insert(key, record);
  };

  // Prints the whole tree out, useful for debugging.
  void print() const { m_root->print(); };

private:
  BPlusNode *m_root;
};
