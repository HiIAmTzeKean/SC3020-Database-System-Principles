#ifndef STORAGE_H
#define STORAGE_H

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "block_storage_impl.h"
#include "data_block.h"

bool stream_just_ended(std::istream &stream);

struct OverflowBlock;
class Node;

class Storage {
public:
  int number_of_records = 0;

  int block_size;

  Storage(const std::string &storage_location, int data_block_count,
          int index_block_count, int overflow_block_count)
      : block_size(get_system_block_size()),
        m_data_blocks(storage_location + "data_", data_block_count),
        m_index_blocks(storage_location + "index_", index_block_count),
        m_overflow_blocks(storage_location + "overflow_",
                          overflow_block_count) {
    m_buffer = new char[block_size]{};
  };
  ~Storage() { delete[] m_buffer; };

  DataBlock *get_data_block(int id);
  Node *get_index_block(int id);
  OverflowBlock *get_overflow_block(int id);

  int track_new_data_block(DataBlock *b);
  int track_new_index_block(Node *b);
  int track_new_overflow_block(OverflowBlock *b);

  size_t loaded_index_block_count() const;
  size_t loaded_data_block_count() const;
  void flush_blocks();

  int write_data_blocks(const std::vector<Record> &records);

private:
  int get_system_block_size();

  BlockStorage<DataBlock> m_data_blocks;
  BlockStorage<Node> m_index_blocks;
  BlockStorage<OverflowBlock> m_overflow_blocks;
  char *m_buffer;
};

#endif // STORAGE_H
