#ifndef NODE_H
#define NODE_H

#include <optional>
#include <vector>

struct RecordPointer {
  int offset;
  int block_id;
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

  float *m_keys;
  union {
    Node **m_node_values;

    struct {
      std::vector<RecordPointer> *m_record_values;
      Node *m_next;
    };
  };
};

#endif // NODE_H
