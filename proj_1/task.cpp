#include "bp_tree.h"
#include <assert.h>
#include <iomanip>

void task_2(BPlusTree *tree) {
  std::cout << "Parameter N: " << tree->get_degree() << std::endl;
  std::cout << "Number of nodes: " << tree->get_number_of_nodes() << std::endl;
  std::cout << "Number of levels: " << tree->get_height() << std::endl;

  std::vector<float> keys = tree->get_root_keys();
  std::cout << "Root keys: [";
  for (size_t i = 0; i < keys.size(); ++i) {
    std::cout << keys[i];
    if (i < keys.size() - 1) {
      std::cout << ", ";
    }
  }
  std::cout << "]" << std::endl;
};

struct Task3Stats {
  double time_taken = 0;
  int num_results = 0;
  float average = 0;

  size_t index_block_count = 0;
  size_t data_block_count = 0;
};

Task3Stats do_bruteforce_scan(Storage *storage, int block_count);
Task3Stats do_bp_tree(BPlusTree *tree);

void task_3(BPlusTree *tree, Storage *storage, int block_count) {
  std::cout << "Task 3: Index Scan vs Brute-Force Linear Scan ('FG_PCT_HOME' "
               "from 0.6 to 0.9, inclusively)"
            << std::endl;
  double bruteforce_time{0}, bp_tree_time{0};
  auto bruteforce_results = do_bruteforce_scan(storage, block_count);
  auto bp_tree_results = do_bp_tree(tree);
  // For time, we do it 1000 times or 30 seconds, whichever comes first, just
  // for good measure.
  int iteration_count = 0;
  while (iteration_count < 1000) {
    ++iteration_count;
    auto bruteforce_time_trial = do_bruteforce_scan(storage, block_count);
    auto bp_tree_time_trial = do_bp_tree(tree);
    bruteforce_time += bruteforce_time_trial.time_taken;
    bp_tree_time += bp_tree_time_trial.time_taken;
    if (bruteforce_time + bp_tree_time > 30) {
      break;
    }
  }

  std::cout << std::setw(16) << std::setiosflags(std::ios::right)
            << std::resetiosflags(std::ios::left);

  std::cout << std::setw(16) << "";
  std::cout << std::setw(12) << "Brute-Force";
  std::cout << std::setw(12) << "B+ Tree" << std::endl;

  std::cout << std::setw(16) << "Time Taken (s)";
  std::cout << std::setw(12) << bruteforce_time;
  std::cout << std::setw(12) << bp_tree_time;
  std::cout << " (" << iteration_count << " iterations)" << std::endl;

  std::cout << std::setw(16) << "Rows Matched";
  std::cout << std::setw(12) << bruteforce_results.num_results;
  std::cout << std::setw(12) << bp_tree_results.num_results;
  std::cout << std::endl;

  std::cout << std::setw(16) << "Avg FG_PCT_HOME";
  std::cout << std::setw(12) << bruteforce_results.average;
  std::cout << std::setw(12) << bp_tree_results.average;
  std::cout << std::endl;

  std::cout << std::setw(16) << "Index Access";
  std::cout << std::setw(12) << bruteforce_results.index_block_count;
  std::cout << std::setw(12) << bp_tree_results.index_block_count;
  std::cout << std::endl;

  std::cout << std::setw(16) << "Data Access";
  std::cout << std::setw(12) << bruteforce_results.data_block_count;
  std::cout << std::setw(12) << bp_tree_results.data_block_count;
  std::cout << std::endl;

  std::cout << std::endl;
  std::cout << std::resetiosflags(std::ios::right);
}

// Actual implementation for task 3.

Task3Stats do_bruteforce_scan(Storage *storage, int block_count) {
  float sum = 0;
  int num_results = 0;

  storage->flush_blocks();
  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < block_count; i++) {
    Block *b = storage->get_data_block(i);
    for (Record record : b->records) {
      if (record.fg_pct_home >= 0.6 && record.fg_pct_home <= 0.9) {
        sum += record.fg_pct_home;
        num_results++;
      }
    }
  }
  auto end_time = std::chrono::high_resolution_clock::now(); // End time
  std::chrono::duration<double> time_taken = end_time - start_time;

  assert(num_results > 0);
  float avg = sum / num_results;

  return Task3Stats{
      .time_taken = time_taken.count(),
      .num_results = num_results,
      .average = avg,
      .index_block_count = storage->loaded_index_block_count(),
      .data_block_count = storage->loaded_data_block_count(),
  };
}

Task3Stats do_bp_tree(BPlusTree *tree) {
  float sum = 0;
  int num_results = 0;

  tree->storage->flush_blocks();
  auto start_time = std::chrono::high_resolution_clock::now();

  for (auto it = tree->search(0.6); it != tree->end(); ++it) {
    assert(it->fg_pct_home >= 0.6);
    if (it->fg_pct_home > 0.9)
      break;
    sum += it->fg_pct_home;
    ++num_results;
  }
  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> time_taken = end_time - start_time;

  assert(num_results > 0);
  float avg = sum / num_results;

  return Task3Stats{
      .time_taken = time_taken.count(),
      .num_results = num_results,
      .average = avg,
      .index_block_count = tree->storage->loaded_index_block_count(),
      .data_block_count = tree->storage->loaded_data_block_count(),
  };
}
