#include "bp_tree.h"
#include "storage/storage.h"
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <optional>
#include <queue>
#include <vector>

int ceil_div(int a, int b) { return (a + b - 1) / b; };

int floor_div(int a, int b) { return a / b; };

void BPlusTree::insert(float key, RecordPointer value) {
  auto optional_created_sibling = root->insert(key, value);
  if (!optional_created_sibling.has_value())
    return;
  auto new_sibling = optional_created_sibling.value();
  auto new_root = new Node(degree, root, new_sibling.key, new_sibling.node);
  root = new_root;
};

// Empty node creation.
Node::Node(int degree, bool is_leaf) : m_is_leaf(is_leaf), m_degree(degree) {
  assert(degree > 2);
  this->m_keys = new float[degree];
  if (is_leaf) {
    this->m_record_values.resize(degree);
    this->m_next = nullptr;
    return;
  }
  auto child_node_count = degree + 1;
  this->m_node_values = new Node *[child_node_count] { nullptr };
};

// Internal node creation.
Node::Node(int degree, Node *a, float key, Node *b) : Node(degree, false) {
  assert(degree > 2);
  assert(a->m_degree = degree);
  assert(b->m_degree = degree);
  this->m_keys[0] = key;
  this->m_node_values[0] = a;
  this->m_node_values[1] = b;
  m_size = 2;
};

Node::~Node() {
  delete[] m_keys;
  if (m_is_leaf)
    return;
  for (int i = 0; i < m_degree + 1; ++i) {
    // Recursively delete nodes.
    delete m_node_values[i];
  }
  delete[] m_node_values;
};

inline size_t Node::key_count() const {
  if (this->m_is_leaf)
    return this->m_size;
  return this->m_size - 1;
}

inline float Node::key_at(int index) const {
  if (this->m_is_leaf)
    assert(index < m_size);
  if (!this->m_is_leaf)
    assert(index < m_size - 1);
  return this->m_keys[index];
}

inline size_t Node::search_key(float key) const {
  if (this->m_is_leaf)
    return std::lower_bound(this->m_keys, this->m_keys + this->key_count(),
                            key) -
           this->m_keys;
  return std::upper_bound(this->m_keys, this->m_keys + this->key_count(), key) -
         this->m_keys;
}

inline std::vector<RecordPointer> Node::records_at(int index) const {
  assert(this->m_is_leaf);
  assert(index < m_size);
  return this->m_record_values[index];
}

inline size_t Node::record_count_at(int index) const {
  assert(this->m_is_leaf);
  assert(index < m_size);
  return this->m_record_values[index].size();
}

inline size_t Node::leaf_entry_count() const {
  assert(this->m_is_leaf);
  return this->m_size;
}

inline Node *Node::child_node_at(int index) const {
  assert(!this->m_is_leaf);
  return this->m_node_values[index];
}

inline size_t Node::child_node_count() const {
  assert(!this->m_is_leaf);
  return this->m_size;
}

std::optional<Node::CreatedSibling> Node::insert(float key,
                                                 RecordPointer record) {
  if (m_is_leaf)
    return insert_leaf(key, record);
  return insert_internal(key, record);
}

std::optional<Node::CreatedSibling> Node::insert_leaf(float key,
                                                      RecordPointer record) {
  assert(m_is_leaf);
  auto keys_end = m_keys + m_size;
  auto it = std::lower_bound(m_keys, keys_end, key);
  auto key_position = it - m_keys;
  if (it != keys_end && *it == key) {
    // For existing keys, just push back.
    m_record_values[key_position].push_back(record);
    return {};
  }
  if (m_size < m_degree) {
    // If we can still fit the key, insert it to the correct location.
    for (auto i = m_size - 1; i >= key_position; --i) {
      // Push every entry back.
      m_keys[i + 1] = m_keys[i];
      m_record_values[i + 1] = m_record_values[i];
    }
    m_keys[key_position] = key;
    m_record_values.at(key_position).clear();
    m_record_values.at(key_position).push_back(record);
    ++m_size;
    return {};
  }
  // Otherwise, split into two nodes.
  Node *sibling = split_leaf_child(key, record);
  return {{.node = sibling, .key = sibling->m_keys[0]}};
}

std::optional<Node::CreatedSibling>
Node::insert_internal(float key, RecordPointer record) {
  assert(!this->m_is_leaf);
  auto key_position = 0;
  while (key_position < m_size - 1 && this->m_keys[key_position] <= key) {
    ++key_position;
  }
  Node *child_for_key = m_node_values[key_position];
  auto optional_new_child = child_for_key->insert(key, record);
  if (!optional_new_child.has_value()) {
    return {};
  }
  // Insert the new node due to overflow into ourselves.
  auto [new_child_node, new_child_key] = optional_new_child.value();
  if (m_size < m_degree + 1) {
    int i;
    for (i = m_size - 2; i >= 0 && m_keys[i] > new_child_key; --i) {
      m_keys[i + 1] = m_keys[i];
      m_node_values[i + 2] = m_node_values[i + 1];
    }
    ++i;
    m_keys[i] = new_child_key;
    m_node_values[i + 1] = new_child_node;
    ++m_size;
    return {};
  }
  return split_internal_child(new_child_key, new_child_node);
};

Node *Node::split_leaf_child(float key, RecordPointer record) {
  assert(this->m_is_leaf);
  Node *sibling = new Node(this->m_degree, true);
  sibling->m_next = this->m_next;
  this->m_next = sibling;

  int split_index = ceil_div(this->m_degree + 1, 2);
  if (key > m_keys[split_index - 1]) {
    // New record should go in second node.
    for (int i = split_index; i < m_size; ++i) {
      sibling->m_keys[i - split_index] = m_keys[i];
      sibling->m_record_values[i - split_index] = m_record_values[i];
      this->m_keys[i] = 0;
      this->m_record_values[i].clear();
    }
    sibling->m_size = this->m_size - split_index;
    this->m_size = split_index;
    auto new_child = sibling->insert_leaf(key, record);
    // Sibling should have enough space to not create a child.
    assert(!new_child.has_value());
    return sibling;
  }

  // New record should go in ourselves.
  --split_index;
  for (int i = split_index; i < m_size; ++i) {
    sibling->m_keys[i - split_index] = m_keys[i];
    sibling->m_record_values[i - split_index] = m_record_values[i];
    m_keys[i] = 0;
    m_record_values[i].clear();
  }
  sibling->m_size = m_size - split_index;
  this->m_size = split_index;
  auto new_child = this->insert_leaf(key, record);
  // We should now have enough space to not create a child.
  assert(!new_child.has_value());
  assert(this->m_keys[0] < sibling->m_keys[0]);
  return sibling;
};

Node::CreatedSibling Node::split_internal_child(float key, Node *record) {
  assert(!this->m_is_leaf);
  Node *sibling = new Node(m_degree, false);
  int split_index = ceil_div(m_degree, 2);
  Node *insert_target_after_split = sibling;
  assert(key != m_keys[split_index - 1]);
  if (key < m_keys[split_index - 1]) {
    insert_target_after_split = this;
    --split_index;
  }
  // Move keys and node values.
  std::copy(this->m_keys + split_index, this->m_keys + m_size - 1,
            sibling->m_keys);
  std::fill(this->m_keys + split_index, this->m_keys + m_size - 1, 0);
  std::copy(this->m_node_values + split_index + 1, this->m_node_values + m_size,
            sibling->m_node_values);
  std::fill(this->m_node_values + split_index + 1, this->m_node_values + m_size,
            nullptr);
  // Update size and insert into the right location.
  sibling->m_size = m_size - split_index - 1;
  this->m_size = split_index + 1;
  if (insert_target_after_split == sibling) {
    auto i = sibling->m_size - 1;
    while (i >= 0 && sibling->m_keys[i] > key) {
      sibling->m_keys[i + 1] = sibling->m_keys[i];
      sibling->m_node_values[i + 1] = sibling->m_node_values[i];
      i--;
    }
    sibling->m_keys[i + 1] = key;
    sibling->m_node_values[i + 1] = record;
    ++sibling->m_size;
  } else {
    auto i = split_index - 1;
    while (i >= 0 && m_keys[i] > key) {
      m_keys[i + 1] = m_keys[i];
      m_node_values[i + 2] = m_node_values[i + 1];
      i--;
    }
    this->m_keys[i + 1] = key;
    this->m_node_values[i + 2] = record;
    ++this->m_size;
  }
  assert(this->m_keys[this->m_size - 1] < sibling->m_keys[0]);
  // shift all sibling keys by 1 to left and move left key up
  float left_key = sibling->m_keys[0];
  for (auto i = 0; i < sibling->m_size - 1; ++i) {
    sibling->m_keys[i] = sibling->m_keys[i + 1];
  }
  return {sibling, left_key};
};

BPlusTree::BPlusTree(Storage *storage, int degree)
    : storage(storage), degree(degree) {
  this->root = new Node(degree);
};

BPlusTree::~BPlusTree() { delete root; };

BPlusTree::Iterator::Iterator(const BPlusTree *tree, Node *node, int index)
    : m_current(node), m_index(index), m_vector_index(0), m_tree(tree) {};

Record *BPlusTree::Iterator::record() const {
  auto record_address = m_current->records_at(m_index)[m_vector_index];
  auto block = m_tree->storage->get_data_block(record_address.block_id);
  return &block->records[record_address.offset];
};

BPlusTree::Iterator &BPlusTree::Iterator::operator++() {
  if (m_current == nullptr)
    return *this;
  ++m_vector_index;
  if (this->m_vector_index < m_current->record_count_at(this->m_index))
    return *this;
  // Advance index in leaf node.
  m_vector_index = 0;
  ++m_index;
  if (this->m_index < m_current->leaf_entry_count())
    return *this;
  // Advance index block.
  m_current = m_current->next_node();
  m_index = 0;
  return *this;
};

bool BPlusTree::Iterator::operator!=(const Iterator &other) const {
  return m_current != other.m_current ||
         m_vector_index != other.m_vector_index || m_index != other.m_index;
};

BPlusTree::Iterator BPlusTree::begin() const {
  Node *current = this->root;
  auto iteration_count = 0;
  while (!current->is_leaf()) {
    current = current->child_node_at(0);
    ++iteration_count;
    assert(iteration_count < MAX_HEIGHT);
  }
  return Iterator(this, current, 0);
}

BPlusTree::Iterator BPlusTree::search(float key) const {
  assert(this->root);
  auto current = this->root;
  auto iteration_count = 0;
  while (!current->is_leaf()) {
    auto index = current->search_key(key);
    assert(index == 0 || current->key_at(index - 1) <= key);
    assert(index == current->key_count() || current->key_at(index) > key);
    current = current->child_node_at(index);
    ++iteration_count;
    assert(iteration_count < MAX_HEIGHT);
  }
  return Iterator(this, current, current->search_key(key));
};

BPlusTree::Iterator BPlusTree::end() const {
  return Iterator(this, nullptr, 0);
};

void BPlusTree::print() {
  if (root) {
    print_node(root, 0);
  }
};

void BPlusTree::print_node(Node *node, int level) {
  if (!node) {
    std::cout << "print_node on VOID!" << std::endl;
    return;
  }

  std::cout << "Level " << level << ": ";

  for (int i = 0; i < node->key_count(); ++i) {
    std::cout << "(" << node->key_at(i) << ") ";
  }

  if (node->is_leaf()) {
    std::cout << "| Size: " << node->leaf_entry_count() << std::endl;
  } else {
    std::cout << "| Size: " << node->child_node_count() << std::endl;
    for (int i = 0; i < node->child_node_count(); ++i) {
      print_node(node->child_node_at(i), level + 1);
    }
  }
};

int BPlusTree::get_height() {
  Node *current = root;
  int height = 1;
  while (!current->is_leaf()) {
    current = current->child_node_at(0);
    ++height;
    assert(height < MAX_HEIGHT);
  }
  return height;
};

std::vector<float> BPlusTree::get_root_keys() {
  std::vector<float> keys;
  for (int i = 0; i < root->key_count(); i++)
    keys.push_back(root->key_at(i));
  return keys;
};

int BPlusTree::get_number_of_nodes() {
  int count = 0;
  std::queue<Node *> nodes_to_visit;
  nodes_to_visit.push(root);
  while (!nodes_to_visit.empty()) {
    ++count;
    Node *current = nodes_to_visit.front();
    nodes_to_visit.pop();
    if (current->is_leaf())
      continue; // We're done with this node!
    for (auto i = 0; i < current->child_node_count(); i++)
      nodes_to_visit.push(current->child_node_at(i));
  }
  return count;
};

void BPlusTree::task_2() {
  std::cout << "Parameter N: " << degree << std::endl;
  std::cout << "Number of nodes: " << this->get_number_of_nodes() << std::endl;
  std::cout << "Number of levels: " << this->get_height() << std::endl;

  std::vector<float> keys = this->get_root_keys();
  std::cout << "Root keys: [";
  for (size_t i = 0; i < keys.size(); ++i) {
    std::cout << keys[i];
    if (i < keys.size() - 1) {
      std::cout << ", ";
    }
  }
  std::cout << "]" << std::endl;
};
