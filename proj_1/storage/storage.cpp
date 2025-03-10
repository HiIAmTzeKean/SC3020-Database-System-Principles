#include "storage.h"
#include "../node.h"
#include "data_block.h"
#include <assert.h>

bool stream_just_ended(std::istream &stream) {
  // Ensure that we haven't read past the end of the file.
  if (stream.eof())
    return false;
  // Attempt to read past end of file and see if we get EOF.
  stream.get();
  return stream.eof();
}

int Storage::write_data_blocks(const std::vector<Record> &records) {
  int max_records_per_block = DataBlock::max_records(this->block_size);
  int total_blocks = 0;
  int total_records = 0;

  DataBlock *block = new DataBlock();
  for (const auto &record : records) {
    block->records.push_back(record);
    if (block->records.size() == max_records_per_block) {
      this->m_data_blocks.track_new_block(block);
      ++total_blocks;
      total_records += block->records.size();
      block = new DataBlock();
    }
  }

  // Serialize partial block
  if (!block->records.empty()) {
    block->id = this->m_data_blocks.track_new_block(block);
    ++total_blocks;
    total_records += block->records.size();
  } else {
    delete block;
  }

  this->m_data_blocks.write_all_cached_blocks();

  std::cout << "Database file written successfully." << std::endl;
  std::cout << "Total blocks written: " << total_blocks << std::endl;
  std::cout << "Total records written: " << total_records << std::endl;

  return total_blocks;
}

size_t Storage::loaded_index_block_count() const {
  return this->m_index_blocks.loaded_block_count();
}
size_t Storage::loaded_data_block_count() const {
  return this->m_data_blocks.loaded_block_count();
}
void Storage::flush_blocks() {
  this->m_index_blocks.write_all_cached_blocks();
  this->m_data_blocks.write_all_cached_blocks();
  this->m_overflow_blocks.write_all_cached_blocks();
  this->m_index_blocks.delete_all_blocks_without_writing();
  this->m_data_blocks.delete_all_blocks_without_writing();
  this->m_overflow_blocks.delete_all_blocks_without_writing();
}

void Storage::flush_cache_without_writing() {
  this->m_index_blocks.delete_all_blocks_without_writing();
  this->m_data_blocks.delete_all_blocks_without_writing();
  this->m_overflow_blocks.delete_all_blocks_without_writing();
}

DataBlock *Storage::get_data_block(int id) {
  return this->m_data_blocks.get(id);
}

Node *Storage::get_index_block(int id) { return this->m_index_blocks.get(id); }

OverflowBlock *Storage::get_overflow_block(int id) {
  return this->m_overflow_blocks.get(id);
}

int Storage::track_new_data_block(DataBlock *b) {
  return this->m_data_blocks.track_new_block(b);
};
int Storage::track_new_index_block(Node *b) {
  return this->m_index_blocks.track_new_block(b);
};
int Storage::track_new_overflow_block(OverflowBlock *b) {
  return this->m_overflow_blocks.track_new_block(b);
};

#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#else
#include <cstdio>
#endif

// Get the current system's block size (page size) in bytes
int Storage::get_system_block_size(void) {
  int block_size;
#ifdef _WIN32
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  block_size = si.dwPageSize;
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
  long res = sysconf(_SC_PAGESIZE);
  block_size = res;
  block_size = 4096;
  if (block_size == -1) {
    block_size = 4096;
    std::cout << "Failed to get system block size, setting block size to "
                 "default of 4096 bytes."
              << std::endl;
  }
#else
  block_size = 4096;
  std::cout << "Failed to get system block size, setting block size to default "
               "of 4096 bytes."
            << std::endl;
#endif

  std::cout << "System block size: " << block_size << " byte" << std::endl;
  return block_size;
}
