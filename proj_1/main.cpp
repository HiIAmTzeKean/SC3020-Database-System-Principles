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
            RecordPointer recordPointer = {record.fg_pct_home, record_offset, 0};
            tree.insert(record.fg_pct_home, &recordPointer);
            record_offset++;
        };
    };
    tree.task_2();

    storage = Storage();
    tree.storage = &storage;
    tree.task_3();
    return 0;
}
