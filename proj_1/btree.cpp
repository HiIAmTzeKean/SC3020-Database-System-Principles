#include "btree.hpp"
#include <algorithm>
#include <assert.h>
#include <iostream>

namespace {
template <typename T>
void unchecked_insert(T array[], int original_length, int insert_index,
                      T value) {
  for (auto i = original_length; i > insert_index; i--) {
    array[i] = array[i - 1];
  }
  array[insert_index] = value;
}

inline int ceil_div(int a, int b) { return (a + b - 1) / b; }
inline int floor_div(int a, int b) { return a / b; }
inline int key_index_to_entry_index(bool is_leaf, int key_index) {
  if (is_leaf)
    return key_index;
  return key_index + 1;
}
} // namespace

BPlusNode::~BPlusNode() {
  if (is_leaf_node)
    return;
  for (auto i = 0; i < entry_count(); i++)
    delete node(i);
}

BPlusNode::Iterator BPlusNode::first() const {
  auto current_node = this;
  while (!current_node->is_leaf_node)
    current_node = current_node->node(0);
  return Iterator(current_node, 0);
}

// NOTE: search will always return the iterator of where the smallest entry
// larger or equal to key is but search_node will return where key belongs in
// the tree.

BPlusNode::Iterator BPlusNode::search(float key) const {
  return search_node(key).canonical();
}

BPlusNode::Iterator BPlusNode::search_node(float key) const {
  auto current_node = this;
  while (!current_node->is_leaf_node) {
    // 1. Find the first element that is larger than the key.
    auto key_count = current_node->m_key_count;
    assert(key_count > 0);
    auto upper_bound = std::upper_bound(current_node->m_keys,
                                        current_node->m_keys + key_count, key);
    auto index = upper_bound - current_node->m_keys;
    // 2. If no such key exists, index will be key_count (last node of the
    //    tree).
    //    If the key exists, we should be looking at the node to the left (same
    //    index as the key).
    current_node = current_node->node(index);
    assert(current_node);
    // 3. Repeat until we are at a leaf node.
  }
  // 4. Search for the key.
  auto key_count = current_node->m_key_count;
  auto lower_bound = std::lower_bound(current_node->m_keys,
                                      current_node->m_keys + key_count, key);
  auto index = lower_bound - current_node->m_keys;
  return Iterator(current_node, index);
};

Record *BPlusNode::search_exact(float key) const {
  auto it = search(key);
  if (it.current_key() == key) {
    return it.record();
  }
  return nullptr;
}

BPlusNode &BPlusNode::insert(float key, Record *record) {
  auto insert_location = search_node(key);
  // The key should not already exists.
  assert(!insert_location.is_valid() || insert_location.current_key() != key);
  auto root = this;
  auto target_node = const_cast<BPlusNode *>(insert_location.m_node);
  auto target_key_index = insert_location.m_index;
  auto target_key = key;
  auto target_entry = Entry{.record = record};
  assert(target_node);
  while (true) {
    auto target_entry_index =
        key_index_to_entry_index(target_node->is_leaf_node, target_key_index);
    // 1. Insert at the index if there are empty space.
    if (target_node->m_key_count < BPlusTreeN) {
      auto entry_count = target_node->is_leaf_node
                             ? target_node->m_key_count
                             : target_node->m_key_count + 1;
      unchecked_insert(target_node->m_keys, target_node->m_key_count,
                       target_key_index, target_key);
      unchecked_insert(target_node->m_entries, entry_count, target_entry_index,
                       target_entry);
      ++target_node->m_key_count;
      // Update the parents' key if this insert changed the lower bounds of the
      // tree.
      auto affected_field = target_key_index;
      auto original_key = target_node->m_keys[1];
      while (target_node->m_parent && affected_field == 0) {
        target_node = target_node->m_parent;
        auto lower_bound = std::lower_bound(
            target_node->m_keys, target_node->m_keys + target_node->m_key_count,
            original_key);
        if (*lower_bound != original_key)
          break;
        *lower_bound = target_key;
        // We can bail early if affected_field is not 0 as we know it's not the
        // smallest subtree in our parent.
        affected_field = lower_bound - target_node->m_keys;
      }
      return *root;
    }
    assert(target_node->m_key_count == BPlusTreeN);
    // 2. Split the node if there are no more empty spaces.
    // a. Original node should now have ceil((N+1)/2) items.
    auto total_entry_count = target_node->entry_count() + 1;
    auto original_node_entry_count = ceil_div(total_entry_count, 2);
    if (target_entry_index < original_node_entry_count) {
      // However, if we are going to insert here, make space while we can.
      --original_node_entry_count;
    }
    target_node->set_entry_count(original_node_entry_count);
    // b. New node should have everything else from the original node.
    //    Minus one as we will insert the new value below.
    auto new_node_entry_count =
        total_entry_count - original_node_entry_count - 1;
    auto new_node = new BPlusNode(target_node->is_leaf_node);
    new_node->set_entry_count(new_node_entry_count);
    // c. Make sure new nodes have the correct values.
    //    The key for the parent is always just the one beyond the end of the
    //    split.
    auto key_to_add_to_parent = target_node->m_keys[target_node->m_key_count];
    auto new_node_keys = target_node->m_keys + original_node_entry_count;
    auto new_node_values = target_node->m_entries + original_node_entry_count;
    std::copy_n(new_node_values, new_node_entry_count, new_node->m_entries);
    std::copy_n(new_node_keys, new_node->m_key_count, new_node->m_keys);
    if (target_node->is_leaf_node) {
      new_node->set_next_ptr(target_node->next_ptr());
      target_node->set_next_ptr(new_node);
    }
    // d. Clear the entries that are no longer valid.
    for (auto i = target_node->m_key_count; i < BPlusTreeN; i++) {
      target_node->m_keys[i] = 0;
      auto entry_index = key_index_to_entry_index(target_node->is_leaf_node, i);
      target_node->m_entries[entry_index].node = nullptr;
      target_node->m_entries[entry_index].record = nullptr;
    }
    // 3. Actually insert the value now that we have the space to!
    if (target_entry_index < ceil_div(BPlusTreeN + 1, 2)) {
      unchecked_insert(target_node->m_keys, target_node->m_key_count,
                       target_key_index, target_key);
      unchecked_insert(target_node->m_entries, target_node->entry_count(),
                       target_entry_index, target_entry);
      ++target_node->m_key_count;
    } else {
      auto key_index_in_new_node = target_key_index - original_node_entry_count;
      auto entry_index_in_new_node = key_index_to_entry_index(
          new_node->is_leaf_node, key_index_in_new_node);
      unchecked_insert(new_node->m_keys, new_node->m_key_count,
                       key_index_in_new_node, target_key);
      unchecked_insert(new_node->m_entries, new_node->entry_count(),
                       entry_index_in_new_node, target_entry);
      ++new_node->m_key_count;
    }
    // 4. Make sure that all nodes have their parents correctly updated.
    if (!new_node->is_leaf_node) {
      for (auto i = 0; i < new_node->entry_count(); i++) {
        new_node->node(i)->m_parent = new_node;
      }
    }
    // 5. Insert the new node to the parent.
    // a. If the parent doesn't exist, we create one!
    //    We also make ourselves our new parent's children.
    auto parent = target_node->m_parent;
    if (!parent) {
      parent = new BPlusNode(false);
      parent->set_entry_count(1);
      parent->m_entries[0].node = target_node;
      target_node->m_parent = parent;
      root = parent;
    }
    new_node->m_parent = parent;
    // b. Find the position to insert the new_node in the parent.
    auto upper_bound =
        std::upper_bound(parent->m_keys, parent->m_keys + parent->m_key_count,
                         key_to_add_to_parent);
    auto index = upper_bound - parent->m_keys;
    // c. Do the insertion by just doing this loop again :D
    target_node = parent;
    target_key_index = index;
    target_entry_index = index + 1;
    target_key = key_to_add_to_parent;
    target_entry = {.node = new_node};
  }
}

void BPlusNode::print() const {
  std::cout << "[";
  if (is_leaf_node) {
    for (auto i = 0; i < entry_count(); i++) {
      std::cout << m_keys[i] << " ";
    }
    std::cout << "] ";
    return;
  }
  for (auto i = 0; i < entry_count(); i++) {
    assert(node(i)->m_parent == this);
    node(i)->print();
    if (i < m_key_count) {
      std::cout << " " << m_keys[i] << " ";
    }
  }
  std::cout << "] ";
}

inline size_t BPlusNode::entry_count() const {
  if (is_leaf_node)
    return m_key_count;
  return m_key_count + 1;
}

inline void BPlusNode::set_entry_count(size_t count) {
  if (is_leaf_node) {
    m_key_count = count;
    return;
  };
  m_key_count = count - 1;
}

inline BPlusNode *BPlusNode::node(size_t index) const {
  assert(!is_leaf_node);
  assert(index >= 0 && index < entry_count());
  auto node = m_entries[index].node;
  assert(node);
  return node;
}

inline BPlusNode *BPlusNode::next_ptr() const {
  assert(is_leaf_node);
  return m_entries[BPlusTreeN].node;
}

inline void BPlusNode::set_next_ptr(BPlusNode *ptr) {
  assert(is_leaf_node);
  m_entries[BPlusTreeN].node = ptr;
}

inline Record *BPlusNode::record(size_t index) const {
  assert(is_leaf_node);
  assert(index >= 0 && index < entry_count());
  return m_entries[index].record;
}

bool BPlusNode::Iterator::is_valid() const {
  return this->m_node && this->m_node->is_leaf_node &&
         this->m_index < this->m_node->entry_count();
};

float BPlusNode::Iterator::current_key() const {
  assert(is_valid());
  return this->m_node->m_keys[this->m_index];
}

BPlusNode::Iterator &BPlusNode::Iterator::canonical() {
  if (is_valid())
    return *this;
  // If it is not valid, it should be pointing to the next node (whether it is
  // nullptr or just the next valid record).
  if (this->m_node) {
    this->m_node = this->m_node->next_ptr();
    this->m_index = 0;
  }
  return *this;
}

BPlusNode::Iterator &BPlusNode::Iterator::operator++() {
  assert(is_valid());
  this->m_index++;
  if (this->m_index < this->m_node->entry_count())
    return *this;
  // Go to the next node.
  // Note that this may create an invalid iterator if we've reached the
  // end.
  this->m_node = this->m_node->next_ptr();
  this->m_index = 0;
  return *this;
};
