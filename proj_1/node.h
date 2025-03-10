#ifndef NODE_H
#define NODE_H

#include "storage/storage.h"
#include <assert.h>
#include <optional>
#include <vector>

constexpr int IN_BLOCK_RECORDS = 8;
constexpr int MAX_OVERFLOW_BLOCKS = 8;

struct RecordPointer {
  int block_id;
  int offset;
};

struct NodePointer {
  int block_id;

  NodePointer() : block_id(-1) {};
  NodePointer(int block_id) : block_id(block_id) {};
  NodePointer(std::istream &stream);
  int serialize(std::ostream &stream) const;
};

struct OverflowBlockPointer {
  int block_id;
};

struct OverflowBlock {
  int id = -1;
  std::vector<RecordPointer> records{};
  std::optional<OverflowBlockPointer> next{};

  OverflowBlock() {};
  OverflowBlock(int block_id, std::istream &stream);
  int serialize(std::ostream &stream) const;
  static size_t max_record_count(size_t block_size);
};

struct NodeRecords {
  int record_count;
  RecordPointer records[IN_BLOCK_RECORDS];
  std::optional<OverflowBlockPointer> more_records;

  NodeRecords() {};
  NodeRecords(std::istream &stream);
  int serialize(std::ostream &stream) const;
  void clear();
  void push_back(Storage *storage, RecordPointer ptr);
};

class Node;
NodePointer create_in_storage(Storage *storage, Node *node);
Node *fetch_from_storage(Storage *storage, NodePointer ptr);

class Node {
public:
  // Create empty leaf node.
  Node(int degree) : Node(degree, true) {};
  // Create internal node.
  Node(int degree, NodePointer a, float key, NodePointer b);
  ~Node();

  Node(int block_id, std::istream &stream);

  int id = -1;
  int serialize(std::ostream &stream) const;

  static size_t max_record_count(size_t block_size);

  struct CreatedSibling {
    NodePointer node;
    float key;
  };

  // Inserts the provided record into self. If it is not possible to fit in the
  // current node, the sibling node created will be returned.
  std::optional<CreatedSibling> insert(Storage *storage, float key,
                                       RecordPointer record);

  inline bool is_leaf() const { return this->m_is_leaf; };
  inline Node *next_node(Storage *storage) const {
    assert(this->m_is_leaf);
    return this->m_next.has_value()
               ? fetch_from_storage(storage, this->m_next.value())
               : nullptr;
  };

  size_t key_count() const;
  float key_at(int index) const;
  // Returns the index where the smallest value that is larger or equal to the
  // key is located. For internal nodes, this returns the index of the node
  // where the key can likely be found. For leaf nodes, this returns where the
  // key is.
  size_t search_key(float key) const;

  std::vector<RecordPointer> records_at(Storage *storage, int index) const;
  size_t leaf_entry_count() const;

  Node *child_node_at(Storage *storage, int index) const;
  size_t child_node_count() const;

private:
  Node(int degree, bool is_leaf);
  static Node create_empty_internal_node();

  std::optional<CreatedSibling> insert_leaf(Storage *storage, float key,
                                            RecordPointer record);
  std::optional<CreatedSibling> insert_internal(Storage *storage, float key,
                                                RecordPointer record);

  CreatedSibling split_leaf_child(Storage *storage, float key,
                                  RecordPointer record);
  CreatedSibling split_internal_child(Storage *storage, float key,
                                      NodePointer record);

  bool m_is_leaf = 0;
  int m_degree = 0;
  int m_size = 0;

  float *m_keys;
  NodePointer *m_node_values;
  NodeRecords *m_record_values;
  std::optional<NodePointer> m_next;
};

#endif // NODE_H
