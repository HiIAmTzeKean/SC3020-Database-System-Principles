#include "bp_tree.h"
#include "storage/storage.h"
#include <assert.h>

int main() {
  std::string inputFile = "games.txt";
  auto storage = Storage();
  std::vector<Record> records = storage.read_records_from_file(inputFile);

  if (records.empty()) {
    std::cerr << "No records found in the file.\n";
    return 1;
  }

  std::cout << std::endl;
  std::cout << "Step 0: Construct Database and Tree" << std::endl;
  std::string outputFile = "data/block_";
  auto block_count = storage.write_database_file(outputFile, records);

  std::vector<Block> blocks;
  for (int i = 0; i < block_count; i++) {
    std::string filename = outputFile + std::to_string(i) + ".dat";
    Block b = storage.read_database_file(filename);
    blocks.push_back(b);
  }

  Storage tree_storage = Storage();
  BPlusTree tree = BPlusTree(&tree_storage, 5);
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
  std::cout << std::endl;

  // Sanity check that the tree is sorted.
  float prev_key = -10000;
  for (auto it = tree.search_range_begin(-10000, 9999);
       it != tree.search_range_end(); ++it) {
    assert(prev_key <= it->fg_pct_home);
    prev_key = it->fg_pct_home;
  }

  std::cout << "Task 1: Storage" << std::endl;
  storage.report_statistics();
  std::cout << std::endl;

  std::cout << "Task 2: B-Tree Statistics" << std::endl;
  tree.task_2();
  std::cout << std::endl;

  // Task 3: Search for rows with attribute “FG_PCT_home” from 0.6 to 0.9 (both
  // inclusively)
  std::cout << "Task 3: Index Scan vs Brute-Force Linear Scan ('FG_PCT_HOME' "
               "from 0.6 to 0.9, inclusively)"
            << std::endl;
  std::cout << "Brute-Force:" << std::endl;
  storage.brute_force_scan(records, 0.6, 0.9);

  std::cout << "Index Scan (B+ tree):" << std::endl;
  tree.task_3();
  return 0;
}
