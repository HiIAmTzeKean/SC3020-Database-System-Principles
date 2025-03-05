#include "node.h"
#include "storage/serialize.h"
#include "storage/storage.h"
#include <algorithm>
#include <assert.h>

int ceil_div(int a, int b) { return (a + b - 1) / b; };

int floor_div(int a, int b) { return a / b; };

OverflowBlock::OverflowBlock(int block_id, std::istream &stream)
    : id(block_id) {
  auto size = Serializer::read_uint32(stream);
  this->records.reserve(size);
  for (auto i = 0; i < size; ++i) {
    auto block_id = Serializer::read_uint32(stream);
    auto offset = Serializer::read_uint16(stream);
    this->records.push_back({.block_id = (int)block_id, .offset = offset});
  }
  auto has_next = Serializer::read_bool(stream);
  if (has_next) {
    auto block_id = Serializer::read_uint32(stream);
    this->next = {{.block_id = (int)block_id}};
  }
  assert(!stream.fail());
  assert(stream_just_ended(stream));
}

int OverflowBlock::serialize(std::ostream &stream) const {
  auto size = 0;
  assert(this->records.size() < 0xFFFFFFFF);
  size += Serializer::write_uint32(stream, this->records.size());
  for (const RecordPointer &record : this->records) {
    assert(record.block_id < 0xFFFFFFFF);
    assert(record.offset < 0xFFFF);
    size += Serializer::write_uint32(stream, record.block_id);
    size += Serializer::write_uint16(stream, record.offset);
  }
  size += Serializer::write_bool(stream, this->next.has_value());
  if (this->next.has_value()) {
    assert(this->next.value().block_id < 0xFFFFFFFF);
    size += Serializer::write_uint32(stream, this->next.value().block_id);
  }
  return size;
}

size_t OverflowBlock::max_record_count(size_t block_size) {
  auto record_size = 4 + 2;
  auto next_block_size = 1 + 4;
  return (block_size - 4 - next_block_size) / record_size;
}

NodeRecords::NodeRecords(std::istream &stream) {
  this->record_count = Serializer::read_uint8(stream);
  assert(this->record_count <= IN_BLOCK_RECORDS);
  for (auto i = 0; i < this->record_count; ++i) {
    auto block_id = Serializer::read_uint32(stream);
    auto offset = Serializer::read_uint16(stream);
    this->records[i] = {.block_id = (int)block_id, .offset = offset};
  }
  auto has_next = Serializer::read_bool(stream);
  if (has_next) {
    auto block_id = Serializer::read_uint32(stream);
    this->more_records = {{.block_id = (int)block_id}};
  } else {
    this->more_records = {};
  }
}

int NodeRecords::serialize(std::ostream &stream) const {
  auto size = 0;
  static_assert(IN_BLOCK_RECORDS < 0xFF, "Record count must fit in uint8");
  size += Serializer::write_uint8(stream, this->record_count);
  for (auto i = 0; i < record_count; ++i) {
    size += Serializer::write_uint32(stream, this->records[i].block_id);
    size += Serializer::write_uint16(stream, this->records[i].offset);
  }
  size += Serializer::write_bool(stream, this->more_records.has_value());
  if (this->more_records.has_value()) {
    auto block_id = this->more_records.value().block_id;
    assert(block_id < 0xFFFFFFFF);
    size += Serializer::write_uint32(stream, block_id);
  }
  return size;
}

void NodeRecords::clear() {
  this->record_count = 0;
  for (auto i = 0; i < IN_BLOCK_RECORDS; ++i)
    this->records[i] = {};
  this->more_records = {};
}

void NodeRecords::push_back(Storage *storage, RecordPointer ptr) {
  if (this->record_count < IN_BLOCK_RECORDS) {
    // Store it in the block.
    this->records[this->record_count] = ptr;
    ++this->record_count;
    return;
  }
  // Follow overflow block towards the end.
  auto max_records_per_overflow_block =
      OverflowBlock::max_record_count(storage->block_size);

  auto new_overflow_location = &this->more_records;
  OverflowBlock *current = nullptr;
  while (new_overflow_location->has_value()) {
    auto overflow_block_id = new_overflow_location->value().block_id;
    current = storage->get_overflow_block(overflow_block_id);
    new_overflow_location = &current->next;
    if (current->records.size() != max_records_per_overflow_block) {
      assert(!current->next.has_value());
      break;
    }
  }
  // If we have space, just write to the overflow block.
  if (current && current->records.size() <
                     current->max_record_count(storage->block_size)) {
    current->records.push_back(ptr);
    return;
  }
  // Otherwise, create a new overflow block.
  auto new_block = new OverflowBlock();
  new_block->records.push_back(ptr);
  *new_overflow_location = {.block_id =
                                storage->track_new_overflow_block(new_block)};
};

// Empty node creation.
Node::Node(int degree, bool is_leaf) : m_is_leaf(is_leaf), m_degree(degree) {
  assert(degree > 2);
  this->m_keys = new float[degree];
  if (is_leaf) {
    this->m_record_values = new NodeRecords[degree];
    this->m_next = {};
    return;
  }
  auto child_node_count = degree + 1;
  this->m_node_values = new NodePointer[child_node_count];
};

// Internal node creation.
Node::Node(int degree, NodePointer a, float key, NodePointer b)
    : Node(degree, false) {
  assert(degree > 2);
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
  delete[] m_node_values;
};

NodePointer::NodePointer(std::istream &stream) {
  auto block_id = Serializer::read_uint32(stream);
  this->block_id = (int)block_id;
}

int NodePointer::serialize(std::ostream &stream) const {
  return Serializer::write_uint32(stream, this->block_id);
}

Node::Node(int block_id, std::istream &stream) : id(block_id) {
  this->m_is_leaf = Serializer::read_bool(stream);
  this->m_degree = Serializer::read_uint16(stream);
  this->m_size = Serializer::read_uint16(stream);
  assert(this->m_size <= this->m_degree + 1);
  this->m_keys = new float[this->key_count()];
  for (auto i = 0; i < this->key_count(); ++i) {
    this->m_keys[i] = Serializer::read_float(stream);
  }
  if (this->m_is_leaf) {
    this->m_record_values = new NodeRecords[this->m_size];
    for (auto i = 0; i < this->m_size; ++i)
      this->m_record_values[i] = NodeRecords(stream);
    auto has_next = Serializer::read_bool(stream);
    if (has_next) {
      this->m_next = {{NodePointer(stream)}};
    } else {
      this->m_next = {};
    }
  } else {
    this->m_node_values = new NodePointer[this->m_size];
    for (auto i = 0; i < this->m_size; ++i)
      this->m_node_values[i] = NodePointer(stream);
  }
  assert(!stream.fail());
  assert(stream_just_ended(stream));
}

int Node::serialize(std::ostream &stream) const {
  auto size = 0;
  size += Serializer::write_bool(stream, this->m_is_leaf);
  assert(this->m_degree < 0xFFFF);
  assert(this->m_size < 0xFFFF);
  size += Serializer::write_uint16(stream, this->m_degree);
  size += Serializer::write_uint16(stream, this->m_size);
  for (auto i = 0; i < this->key_count(); ++i)
    Serializer::write_float(stream, this->m_keys[i]);
  if (this->m_is_leaf) {
    for (auto i = 0; i < this->m_size; ++i)
      size += this->m_record_values[i].serialize(stream);
    size += Serializer::write_bool(stream, this->m_next.has_value());
    if (this->m_next.has_value())
      size += this->m_next.value().serialize(stream);
  } else {
    for (auto i = 0; i < this->m_size; ++i)
      size += this->m_node_values[i].serialize(stream);
  }
  return size;
}

// NOTE: create_in_storage takes over ownership of the node pointer.
NodePointer create_in_storage(Storage *storage, Node *node) {
  return NodePointer(storage->track_new_index_block(node));
};

Node *fetch_from_storage(Storage *storage, NodePointer ptr) {
  return storage->get_index_block(ptr.block_id);
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

std::vector<RecordPointer> Node::records_at(Storage *storage, int index) const {
  assert(this->m_is_leaf);
  assert(index < m_size);
  std::vector<RecordPointer> records;

  auto record_values = this->m_record_values[index];
  auto count = record_values.record_count;
  assert(count <= IN_BLOCK_RECORDS);
  for (auto i = 0; i < count; ++i)
    records.push_back(record_values.records[i]);
  // Follow overflow blocks.
  auto overflow_block = record_values.more_records;
  for (auto i = 0; i <= MAX_OVERFLOW_BLOCKS && overflow_block.has_value();
       ++i) {
    assert(i != MAX_OVERFLOW_BLOCKS);
    auto block = storage->get_overflow_block(overflow_block.value().block_id);
    for (auto j = 0; j < block->records.size(); ++j)
      records.push_back(block->records[j]);
    overflow_block = block->next;
  }
  return records;
}

size_t Node::leaf_entry_count() const {
  assert(this->m_is_leaf);
  return this->m_size;
}

Node *Node::child_node_at(Storage *storage, int index) const {
  assert(!this->m_is_leaf);
  return fetch_from_storage(storage, this->m_node_values[index]);
}

size_t Node::child_node_count() const {
  assert(!this->m_is_leaf);
  return this->m_size;
}

std::optional<Node::CreatedSibling> Node::insert(Storage *storage, float key,
                                                 RecordPointer record) {
  if (m_is_leaf)
    return insert_leaf(storage, key, record);
  return insert_internal(storage, key, record);
}

std::optional<Node::CreatedSibling>
Node::insert_leaf(Storage *storage, float key, RecordPointer record) {
  assert(m_is_leaf);
  auto keys_end = m_keys + m_size;
  auto it = std::lower_bound(m_keys, keys_end, key);
  auto key_position = it - m_keys;
  if (it != keys_end && *it == key) {
    // For existing keys, just push back.
    m_record_values[key_position].push_back(storage, record);
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
    m_record_values[key_position].push_back(storage, record);
    ++m_size;
    return {};
  }
  // Otherwise, split into two nodes.
  return split_leaf_child(storage, key, record);
}

std::optional<Node::CreatedSibling>
Node::insert_internal(Storage *storage, float key, RecordPointer record) {
  assert(!this->m_is_leaf);
  auto key_position = 0;
  while (key_position < m_size - 1 && this->m_keys[key_position] <= key) {
    ++key_position;
  }
  Node *child_for_key =
      fetch_from_storage(storage, m_node_values[key_position]);
  auto optional_new_child = child_for_key->insert(storage, key, record);
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
  return split_internal_child(storage, new_child_key, new_child_node);
};

Node::CreatedSibling Node::split_leaf_child(Storage *storage, float key,
                                            RecordPointer record) {
  assert(this->m_is_leaf);
  Node *sibling = new Node(this->m_degree, true);
  sibling->m_next = this->m_next;
  auto sibling_pointer = create_in_storage(storage, sibling);
  this->m_next = sibling_pointer;

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
    auto new_child = sibling->insert_leaf(storage, key, record);
    // Sibling should have enough space to not create a child.
    assert(!new_child.has_value());
    return {.node = sibling_pointer, .key = sibling->m_keys[0]};
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
  auto new_child = this->insert_leaf(storage, key, record);
  // We should now have enough space to not create a child.
  assert(!new_child.has_value());
  assert(this->m_keys[0] < sibling->m_keys[0]);
  return {.node = sibling_pointer, .key = sibling->m_keys[0]};
};

Node::CreatedSibling Node::split_internal_child(Storage *storage, float key,
                                                NodePointer record) {
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
            NodePointer(-1));
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
  return {.node = create_in_storage(storage, sibling), .key = left_key};
};
