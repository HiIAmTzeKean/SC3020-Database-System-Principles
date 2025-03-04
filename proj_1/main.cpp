#include "bp_tree.h"
#include "storage/storage.h"
#include <assert.h>

int main() {
  std::string inputFile = "games.txt";
  Storage storage;
  std::vector<Record> records = storage.readRecordsFromFile(inputFile);

  if (records.empty()) {
    std::cerr << "No records found in the file.\n";
    return 1;
  }

  std::string outputFile = "data/block_";
  auto block_count = storage.writeDatabaseFile(outputFile, records);

  std::vector<Block> blocks;
  for (int i = 0; i < block_count; i++) {
    std::string filename = outputFile + std::to_string(i) + ".dat";
    Block b = storage.readDatabaseFile(filename);
    blocks.push_back(b);
  }

  // Task 1: Storage component
  storage.reportStatistics();

  // Task 3: Search for rows with attribute “FG_PCT_home” from 0.6 to 0.9 (both
  // inclusively)
  storage.bruteForceScan(records, 0.6, 0.9);

  storage = Storage();
  BPlusTree tree = BPlusTree(5);
  tree.storage = &storage;
  for (Block &block : blocks) {
    int record_offset = 0;
    for (Record &record : block.records) {
      RecordPointer recordPointer = {.key = record.fg_pct_home,
                                     .offset = record_offset,
                                     .block_id = block.id};
      tree.insert(record.fg_pct_home, recordPointer);
      ++record_offset;
    };
  };

  // Sanity check that the tree is sorted.
  float prev_key = -10000;
  for (auto it = tree.search_range_begin(-10000, 9999);
       it != tree.search_range_end(); ++it) {
    assert(prev_key <= it->fg_pct_home);
    prev_key = it->fg_pct_home;
  }

  tree.task_2();
  tree.task_3();
  return 0;
}
