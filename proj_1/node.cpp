#include "node.h"
#include <algorithm>
#include <assert.h>

int ceil_div(int a, int b) { return (a + b - 1) / b; };

int floor_div(int a, int b) { return a / b; };

// Empty node creation.
Node::Node(int degree, bool is_leaf) : m_is_leaf(is_leaf), m_degree(degree) {
  assert(degree > 2);
  this->m_keys = new float[degree];
  if (is_leaf) {
    this->m_record_values = new std::vector<RecordPointer>[degree];
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
  if (m_is_leaf) {
    delete[] m_record_values;
    return;
  }
  for (int i = 0; i < m_degree + 1; ++i) {
    // Recursively delete nodes.
    delete m_node_values[i];
  }
  delete[] m_node_values;
};

size_t Node::key_count() const {
  if (this->m_is_leaf)
    return this->m_size;
  return this->m_size - 1;
}

float Node::key_at(int index) const {
  if (this->m_is_leaf)
    assert(index < m_size);
  if (!this->m_is_leaf)
    assert(index < m_size - 1);
  return this->m_keys[index];
}

size_t Node::search_key(float key) const {
  if (this->m_is_leaf)
    return std::lower_bound(this->m_keys, this->m_keys + this->key_count(),
                            key) -
           this->m_keys;
  return std::upper_bound(this->m_keys, this->m_keys + this->key_count(), key) -
         this->m_keys;
}

std::vector<RecordPointer> Node::records_at(int index) const {
  assert(this->m_is_leaf);
  assert(index < m_size);
  return this->m_record_values[index];
}

size_t Node::record_count_at(int index) const {
  assert(this->m_is_leaf);
  assert(index < m_size);
  return this->m_record_values[index].size();
}

size_t Node::leaf_entry_count() const {
  assert(this->m_is_leaf);
  return this->m_size;
}

Node *Node::child_node_at(int index) const {
  assert(!this->m_is_leaf);
  return this->m_node_values[index];
}

size_t Node::child_node_count() const {
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
    m_record_values[key_position].clear();
    m_record_values[key_position].push_back(record);
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
