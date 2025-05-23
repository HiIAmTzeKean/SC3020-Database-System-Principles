#include "bp_tree.h"
#include "storage/data_block.h"
#include "storage/storage.h"
#include <assert.h>
#include <iostream>
#include <queue>
#include <vector>

void BPlusTree::insert(float key, RecordPointer value) {
  auto optional_created_sibling = fetch_from_storage(this->storage, m_root)
                                      ->insert(this->storage, key, value);
  if (!optional_created_sibling.has_value())
    return;
  auto new_sibling = optional_created_sibling.value();
  auto new_root = create_in_storage(
      storage, new Node(m_degree, m_root, new_sibling.key, new_sibling.node));
  m_root = new_root;
};

BPlusTree::BPlusTree(Storage *storage, int degree)
    : storage(storage), m_degree(degree) {
  this->m_root = create_in_storage(storage, new Node(degree));
};

BPlusTree::Iterator::Iterator(const BPlusTree *tree, Node *node, int index)
    : m_current(node), m_index(index), m_vector_index(0), m_tree(tree) {};

Record *BPlusTree::Iterator::record() const {
  // TODO: Reconstructing vector every single time is very inefficient.
  auto records = m_current->records_at(this->m_tree->storage, this->m_index);
  assert(this->m_vector_index < records.size());
  auto record_address = records[this->m_vector_index];
  auto block = m_tree->storage->get_data_block(record_address.block_id);
  return &block->records[record_address.offset];
};

BPlusTree::Iterator &BPlusTree::Iterator::operator++() {
  if (m_current == nullptr)
    return *this;
  ++m_vector_index;
  // TODO: Reconstructing vector every single time is very inefficient.
  if (this->m_vector_index <
      m_current->records_at(this->m_tree->storage, this->m_index).size())
    return *this;
  // Advance index in leaf node.
  m_vector_index = 0;
  ++m_index;
  if (this->m_index < m_current->leaf_entry_count())
    return *this;
  // Advance index block.
  m_current = m_current->next_node(this->m_tree->storage);
  m_index = 0;
  return *this;
};

bool BPlusTree::Iterator::operator!=(const Iterator &other) const {
  return m_current != other.m_current ||
         m_vector_index != other.m_vector_index || m_index != other.m_index;
};

BPlusTree::Iterator BPlusTree::begin() const {
  Node *current = fetch_from_storage(this->storage, this->m_root);
  auto iteration_count = 0;
  while (!current->is_leaf()) {
    current = current->child_node_at(this->storage, 0);
    ++iteration_count;
    assert(iteration_count < MAX_HEIGHT);
  }
  return Iterator(this, current, 0);
}

BPlusTree::Iterator BPlusTree::search(float key) const {
  auto current = fetch_from_storage(this->storage, this->m_root);
  auto iteration_count = 0;
  while (!current->is_leaf()) {
    auto index = current->search_key(key);
    assert(index == 0 || current->key_at(index - 1) <= key);
    assert(index == current->key_count() || current->key_at(index) > key);
    current = current->child_node_at(this->storage, index);
    ++iteration_count;
    assert(iteration_count < MAX_HEIGHT);
  }
  return Iterator(this, current, current->search_key(key));
};

BPlusTree::Iterator BPlusTree::end() const {
  return Iterator(this, nullptr, 0);
};

void BPlusTree::print() {
  print_node(fetch_from_storage(this->storage, this->m_root), 0);
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
      print_node(node->child_node_at(this->storage, i), level + 1);
    }
  }
};

int BPlusTree::get_height() {
  Node *current = fetch_from_storage(this->storage, this->m_root);
  int height = 1;
  while (!current->is_leaf()) {
    current = current->child_node_at(this->storage, 0);
    ++height;
    assert(height < MAX_HEIGHT);
  }
  return height;
};

std::vector<float> BPlusTree::get_root_keys() {
  auto root = fetch_from_storage(this->storage, this->m_root);
  std::vector<float> keys;
  for (int i = 0; i < root->key_count(); i++)
    keys.push_back(root->key_at(i));
  return keys;
};

int BPlusTree::get_number_of_nodes() {
  int count = 0;
  std::queue<Node *> nodes_to_visit;
  nodes_to_visit.push(fetch_from_storage(this->storage, this->m_root));
  while (!nodes_to_visit.empty()) {
    ++count;
    Node *current = nodes_to_visit.front();
    nodes_to_visit.pop();
    if (current->is_leaf())
      continue; // We're done with this node!
    for (auto i = 0; i < current->child_node_count(); i++)
      nodes_to_visit.push(current->child_node_at(this->storage, i));
  }
  return count;
};
