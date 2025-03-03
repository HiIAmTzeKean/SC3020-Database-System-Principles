#include "storage/storage.h"
#include "bp_tree.h"
int main() {
    std::string inputFile = "games.txt";
    Storage storage;
    std::vector<Record> records = storage.readRecordsFromFile(inputFile);

    if (records.empty()) {
        std::cerr << "No records found in the file.\n";
        return 1;
    }

    std::string outputFile = "data/block_";
    storage.writeDatabaseFile(outputFile, records);

    // std::vector<Record> loadedRecords;
    std::vector<Block> blocks;
    for (int i = 0; i < 46; i++) {
        std::string filename = outputFile + std::to_string(i) + ".dat";
        Block b = storage.readDatabaseFile(filename);
        blocks.push_back(b);
    }

    // Task 1: Storage component
    // storage.reportStatistics();

    // Task 3: Search for rows with attribute “FG_PCT_home” from 0.6 to 0.9 (both inclusively)
    storage.bruteForceScan(records, 0.6, 0.9);

    BPlusTree tree = BPlusTree(5);
    for (Block &block : blocks) {
        int record_offset = 0;
        for (Record &record : block.records) {
            // if (record.fg_pct_home == 0.6f) {
            //     std::cout << "hit" << std::endl;
            // }
            RecordPointer recordPointer = {record.fg_pct_home, record_offset, 0};
            tree.insert(record.fg_pct_home, &recordPointer);
            record_offset++;
        };
    };
    tree.task_2();

    delete &storage;
    Storage storage = Storage();
    tree.storage = &storage;
    tree.task_3();
    // int number_of_target = 0;
    // std::vector<Record*> searchResults = tree.search(0.6f);
    // for (Record* r : searchResults) {
    //     std::cout << r->fg_pct_home << " " << r->ft_pct_home << std::endl;
    //     number_of_target++;
    // };
    // std::cout << "Number of target: " << number_of_target << std::endl;
    // searchResults = tree.search(30);
    // for (Record* r : searchResults) {
    //     std::cout << r->fg_pct_home << " " << r->ft_pct_home << std::endl;
    // };

    return 0;
}
