#include "bp_tree.h"
#include "storage/storage.h"
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <optional>
#include <utility>
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
Node::Node(int degree, bool is_leaf) : is_leaf(is_leaf), degree(degree) {
  assert(degree > 2);
  this->keys = new float[degree];
  if (is_leaf) {
    this->record_values.resize(degree);
    this->next = nullptr;
    return;
  }
  auto child_node_count = degree + 1;
  this->node_values = new Node *[child_node_count] { nullptr };
};

// Internal node creation.
Node::Node(int degree, Node *a, float key, Node *b) : Node(degree, false) {
  assert(degree > 2);
  assert(a->degree = degree);
  assert(b->degree = degree);
  this->keys[0] = key;
  this->node_values[0] = a;
  this->node_values[1] = b;
  size = 2;
};

Node::~Node() {
  delete[] keys;
  if (is_leaf) {
    // delete[] record_values;
  } else {
    for (int i = 0; i < degree + 1; ++i) {
      delete node_values[i];
    }
    delete[] node_values;
  }
};

std::optional<Node::CreatedSibling> Node::insert(float key,
                                                 RecordPointer record) {
  if (is_leaf)
    return insert_leaf(key, record);
  return insert_internal(key, record);
}

std::optional<Node::CreatedSibling> Node::insert_leaf(float key,
                                                      RecordPointer record) {
  assert(is_leaf);
  auto keys_end = keys + size;
  auto it = std::lower_bound(keys, keys_end, key);
  auto key_position = it - keys;
  if (it != keys_end && *it == key) {
    // For existing keys, just push back.
    record_values[key_position].push_back(record);
    return {};
  }
  if (size < degree) {
    // If we can still fit the key, insert it to the correct location.
    for (auto i = size - 1; i >= key_position; --i) {
      // Push every entry back.
      keys[i + 1] = keys[i];
      record_values[i + 1] = record_values[i];
    }
    keys[key_position] = key;
    record_values.at(key_position).clear();
    record_values.at(key_position).push_back(record);
    size++;
    return {};
  }
  // Otherwise, split into two nodes.
  Node *sibling = split_leaf_child(key, record);
  return {{.node = sibling, .key = sibling->keys[0]}};
}

std::optional<Node::CreatedSibling>
Node::insert_internal(float key, RecordPointer record) {
  assert(!is_leaf);
  auto key_position = 0;
  while (key_position < size - 1 && keys[key_position] <= key) {
    ++key_position;
  }
  Node *child_for_key = node_values[key_position];
  auto optional_new_child = child_for_key->insert(key, record);
  if (!optional_new_child.has_value()) {
    return {};
  }
  // Insert the new node due to overflow into ourselves.
  auto [new_child_node, new_child_key] = optional_new_child.value();
  if (size < degree + 1) {
    int i;
    for (i = size - 2; i >= 0 && keys[i] > new_child_key; --i) {
      keys[i + 1] = keys[i];
      node_values[i + 2] = node_values[i + 1];
    }
    ++i;
    keys[i] = new_child_key;
    node_values[i + 1] = new_child_node;
    ++size;
    return {};
  }
  return split_internal_child(new_child_key, new_child_node);
};

Node *Node::split_leaf_child(float key, RecordPointer record) {
  assert(this->is_leaf);
  Node *sibling = new Node(this->degree, true);
  sibling->next = this->next;
  this->next = sibling;

  int split_index = ceil_div(this->degree + 1, 2);
  if (key > keys[split_index - 1]) {
    // New record should go in second node.
    for (int i = split_index; i < size; i++) {
      sibling->keys[i - split_index] = keys[i];
      sibling->record_values[i - split_index] = record_values[i];
      this->keys[i] = 0;
      this->record_values[i].clear();
    }
    sibling->size = this->size - split_index;
    this->size = split_index;
    auto new_child = sibling->insert_leaf(key, record);
    // Sibling should have enough space to not create a child.
    assert(!new_child.has_value());
    return sibling;
  }

  // New record should go in ourselves.
  --split_index;
  for (int i = split_index; i < size; ++i) {
    sibling->keys[i - split_index] = keys[i];
    sibling->record_values[i - split_index] = record_values[i];
    keys[i] = 0;
    record_values[i].clear();
  }
  sibling->size = size - split_index;
  this->size = split_index;
  auto new_child = this->insert_leaf(key, record);
  // We should now have enough space to not create a child.
  assert(!new_child.has_value());
  assert(this->keys[0] < sibling->keys[0]);
  return sibling;
};

Node::CreatedSibling Node::split_internal_child(float key, Node *record) {
  assert(!this->is_leaf);
  Node *sibling = new Node(degree, false);
  int split_index = ceil_div(degree, 2);
  Node *insert_target_after_split = sibling;
  assert(key != keys[split_index - 1]);
  if (key < keys[split_index - 1]) {
    insert_target_after_split = this;
    --split_index;
  }
  // Move keys and node values.
  std::copy(this->keys + split_index, this->keys + size - 1, sibling->keys);
  std::fill(this->keys + split_index, this->keys + size - 1, 0);
  std::copy(this->node_values + split_index + 1, this->node_values + size,
            sibling->node_values);
  std::fill(this->node_values + split_index + 1, this->node_values + size,
            nullptr);
  // Update size and insert into the right location.
  sibling->size = size - split_index - 1;
  this->size = split_index + 1;
  if (insert_target_after_split == sibling) {
    auto i = sibling->size - 1;
    while (i >= 0 && sibling->keys[i] > key) {
      sibling->keys[i + 1] = sibling->keys[i];
      sibling->node_values[i + 1] = sibling->node_values[i];
      i--;
    }
    sibling->keys[i + 1] = key;
    sibling->node_values[i + 1] = record;
    ++sibling->size;
  } else {
    auto i = split_index - 1;
    while (i >= 0 && keys[i] > key) {
      keys[i + 1] = keys[i];
      node_values[i + 2] = node_values[i + 1];
      i--;
    }
    this->keys[i + 1] = key;
    this->node_values[i + 2] = record;
    ++this->size;
  }
  assert(this->keys[this->size - 1] < sibling->keys[0]);
  // shift all sibling keys by 1 to left and move left key up
  float left_key = sibling->keys[0];
  for (auto i = 0; i < sibling->size - 1; i++) {
    sibling->keys[i] = sibling->keys[i + 1];
  }
  return {sibling, left_key};
};

BPlusTree::BPlusTree(Storage *storage, int degree)
    : storage(storage), degree(degree) {
  this->root = new Node(degree);
};

BPlusTree::~BPlusTree() { delete root; };

BPlusTree::Iterator::Iterator(Node *node, int index, float right_key,
                              BPlusTree *tree)
    : current(node), index(index), right_key(right_key), tree(tree) {};

Record *BPlusTree::Iterator::record() const {
  auto record_address = current->record_values[index][vector_index];
  auto block = tree->storage->read_block(record_address.block_id);
  return &block->records[record_address.offset];
};

BPlusTree::Iterator &BPlusTree::Iterator::operator++() {
  if (current == nullptr)
    return *this;
  ++vector_index;
  if (vector_index < current->record_values[index].size())
    return *this;
  // Advance index in leaf node.
  vector_index = 0;
  ++index;
  if (index < current->size)
    return *this;
  // Advance index block.
  ++tree->index_block_hit;
  current = current->next;
  index = 0;
  return *this;
};

bool BPlusTree::Iterator::operator!=(const Iterator &other) const {
  return current != other.current && current->keys[index] <= right_key;
};

BPlusTree::Iterator BPlusTree::search_range_begin(float left_key,
                                                  float right_key) {
  Node *current = search_leaf_node(left_key);
  if (current == nullptr) {
    return Iterator(nullptr, 0, right_key, this);
  }
  int i = 0;
  while (i < current->size && current->keys[i] < left_key) {
    i++;
  }
  return Iterator(current, i, right_key, this);
};

BPlusTree::Iterator BPlusTree::search_range_end() {
  return Iterator(nullptr, 0, 0, this);
};

int BPlusTree::get_index(float key, Node *node) {
  for (int i = 0; i < node->size && node->keys[i] <= key; i++) {
    if (node->keys[i] == key) {
      return i;
    }
  }
  return -1;
};

Node *BPlusTree::search_leaf_node(float key) {
  if (root == nullptr) {
    return nullptr;
  }
  Node *current = root;
  index_block_hit++; // for root

  while (!current->is_leaf) {
    int i = 0;
    while (i < current->size - 1 && current->keys[i] <= key) {
      i++;
    }
    current = current->node_values[i];
    index_block_hit++; // for internal node
  }
  return current;
};

std::pair<BPlusTree::Iterator, BPlusTree::Iterator>
BPlusTree::search_range_iter(float left_key, float right_key) {
  return std::make_pair(search_range_begin(left_key, right_key),
                        search_range_end());
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

  if (node->is_leaf) {
    for (int i = 0; i < node->size; ++i) {
      std::cout << "(" << node->keys[i] << ") ";
    }
  } else {
    for (int i = 0; i < node->size - 1; ++i) {
      std::cout << "(" << node->keys[i] << ") ";
    }
  }

  std::cout << "| Size: " << node->size << std::endl;

  if (!node->is_leaf) {
    for (int i = 0; i < node->size; ++i) {
      print_node(node->node_values[i], level + 1);
    }
  }
};

int BPlusTree::get_height() {
  Node *current = root;
  int height = 1;
  while (!current->is_leaf) {
    current = current->node_values[0];
    height++;
  }
  return height;
};
std::vector<float> BPlusTree::get_root_keys() {
  std::vector<float> keys;
  for (int i = 0; i < root->size - 1; i++) {
    keys.push_back(root->keys[i]);
  }
  return keys;
};
int BPlusTree::get_number_of_nodes() {
  int count = 0;
  std::vector<Node *> current_level;
  current_level.push_back(root);
  while (!current_level.empty()) {
    std::vector<Node *> next_level;
    for (Node *node : current_level) {
      if (node == nullptr) {
        continue;
      }
      count++;
      if (!node->is_leaf) {
        for (int i = 0; i < node->size; i++) {
          next_level.push_back(node->node_values[i]);
        }
      }
    }
    current_level = next_level;
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

void BPlusTree::task_3() {
  float sum = 0;
  int num_results = 0;
  auto start_time = std::chrono::high_resolution_clock::now();

  auto [begin, end] = this->search_range_iter(0.6, 0.9);
  for (auto it = begin; it != end; ++it) {
    assert(it->fg_pct_home >= 0.6);
    assert(it->fg_pct_home <= 0.9);
    sum += it->fg_pct_home;
    num_results++;
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> time_taken = end_time - start_time;
  std::cout << "B+ Tree scan time: " << time_taken.count() << " seconds"
            << '\n';

  std::cout << "Number of records found in range: " << num_results << '\n';
  if (num_results > 0) {
    float avg = sum / num_results;
    std::cout << "Average of 'FG_PCT_home': " << avg << '\n';
  }

  std::cout << "Number of index blocks accessed in tree: "
            << this->index_block_hit << '\n';
  std::cout << "Number of data blocks accessed in tree: "
            << this->storage->loaded_data_block_count() << '\n';
}
