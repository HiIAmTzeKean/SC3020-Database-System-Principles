#include "bp_tree.h"
#include "storage/data_block.h"
#include "storage/storage.h"
#include "task.h"
#include <assert.h>

int main() {
  std::string inputFile = "games.txt";
  auto storage = Storage("data/block_", 0, 0, 0);
  std::vector<Record> records = read_records_from_file(inputFile);

  if (records.empty()) {
    std::cerr << "No records found in the file.\n";
    return 1;
  }

  std::cout << std::endl;
  std::cout << "Step 0: Construct Database and Tree" << std::endl;
  auto block_count = storage.write_data_blocks(records);

  BPlusTree tree = BPlusTree(&storage, 5);
  for (int i = 0; i < block_count; i++) {
    DataBlock b = *storage.get_data_block(i);
    int record_offset = 0;
    for (Record &record : b.records) {
      RecordPointer recordPointer = {.block_id = b.id, .offset = record_offset};
      tree.insert(record.fg_pct_home, recordPointer);
      ++record_offset;
    };
  }
  std::cout << std::endl;
  storage.flush_blocks();

  // Sanity check that the tree is sorted.
  float prev_key = -10000;
  for (auto it = tree.begin(); it != tree.end(); ++it) {
    assert(prev_key <= it->fg_pct_home);
    prev_key = it->fg_pct_home;
  }

  std::cout << "Task 1: Storage" << std::endl;
  task_1(&storage);
  std::cout << std::endl;

  std::cout << "Task 2: B-Tree Statistics" << std::endl;
  task_2(&tree);
  std::cout << std::endl;

  task_3(&tree, &storage, block_count);

  return 0;
}
