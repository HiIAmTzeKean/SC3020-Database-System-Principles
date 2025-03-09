#ifndef BLOCK_STORAGE_IMPL_H
#define BLOCK_STORAGE_IMPL_H

#include <assert.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

template <typename T> class BlockStorage {
public:
  BlockStorage(const std::string &storage_prefix, int block_count)
      : m_storage_prefix(storage_prefix), m_total_block_count(block_count) {};
  ~BlockStorage() { delete_all_blocks_without_writing(); };

  T *get(int block_id);

  int track_new_block(T *value);
  void write_all_cached_blocks();
  void delete_all_blocks_without_writing();

  int loaded_block_count() const { return this->m_cached_entries.size(); };

private:
  const std::string block_location(int block_id) const;
  void read_block(int block_id);

  std::map<int, T *> m_cached_entries;
  const std::string m_storage_prefix;
  int m_total_block_count;
};

template <typename T> T *BlockStorage<T>::get(int block_id) {
  auto it = this->m_cached_entries.find(block_id);
  if (it != this->m_cached_entries.end())
    return it->second;
  this->read_block(block_id);
  // It should be cached now.
  it = this->m_cached_entries.find(block_id);
  assert(it != this->m_cached_entries.end());
  return it->second;
}

template <typename T> void BlockStorage<T>::read_block(int block_id) {
  assert(this->m_cached_entries.find(block_id) == this->m_cached_entries.end());
  assert(block_id >= 0);
  std::ifstream file(block_location(block_id), std::ios::binary);
  if (!file)
    throw std::runtime_error("Error opening file for reading.");

  auto block = new T(block_id, file);
  file.close();

  this->m_cached_entries.insert_or_assign(block_id, block);
}

template <typename T> int BlockStorage<T>::track_new_block(T *value) {
  value->id = this->m_total_block_count;
  this->m_cached_entries.insert_or_assign(this->m_total_block_count, value);
  ++this->m_total_block_count;
  return this->m_total_block_count - 1;
}

template <typename T> void BlockStorage<T>::write_all_cached_blocks() {
  for (auto it = this->m_cached_entries.begin();
       it != this->m_cached_entries.end(); ++it) {
    auto block = it->second;
    assert(it->first >= 0);
    assert(block->id == it->first);
    std::ofstream file(block_location(block->id), std::ios::binary);
    if (!file) {
      std::cerr << "Error opening file for writing." << std::endl;
      return;
    }
    block->serialize(file);
    file.close();
  }
}

template <typename T>
void BlockStorage<T>::delete_all_blocks_without_writing() {
  for (auto it = this->m_cached_entries.begin();
       it != this->m_cached_entries.end(); ++it)
    delete it->second;
  this->m_cached_entries.clear();
}

template <typename T>
const std::string BlockStorage<T>::block_location(int block_id) const {
  return this->m_storage_prefix + std::to_string(block_id) + ".dat";
}

#endif // BLOCK_STORAGE_IMPL_H
