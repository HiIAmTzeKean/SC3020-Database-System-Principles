#include "btree.hpp"
#include "iostream"

int main() {
  auto tree = BPlusTree();
  std::cout << "Constructing tree by inserting values from 100 to 0. (25 & 75 "
               "are skipped)"
            << std::endl;
  for (auto i = 100; i >= 0; --i) {
    if (i == 25 || i == 75)
      continue;
    tree.insert(i, nullptr);
  }
  std::cout << "Doing range search of [0, infinity)" << std::endl;
  for (auto it = tree.search(0); it.is_valid(); ++it) {
    std::cout << it.current_key() << " ";
  }
  std::cout << std::endl;
  std::cout << "Doing range search of [50, infinity)" << std::endl;
  for (auto it = tree.search(50); it.is_valid(); ++it) {
    std::cout << it.current_key() << " ";
  }
  std::cout << std::endl;
  std::cout << "Doing range search of [25, 75)" << std::endl;
  for (auto it = tree.search(25); it != tree.search(75); ++it) {
    std::cout << it.current_key() << " ";
  }
  std::cout << std::endl;
  std::cout << "Doing range search of (-infinity, 42)" << std::endl;
  for (auto it = tree.first(); it != tree.search(42); ++it) {
    std::cout << it.current_key() << " ";
  }
  std::cout << std::endl;
}
