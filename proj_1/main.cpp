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

    BPlusTree tree = BPlusTree(3);
    for (Record &record : records) {
        tree.insert(record.fg_pct_home, &record);
        tree.print();
    }

    // std::string outputFile = "nba_database.dat";
    // storage.writeDatabaseFile(outputFile, records);

    // std::vector<Record> loadedRecords;
    // storage.readDatabaseFile(outputFile, loadedRecords);

    // // Task 1: Storage component
    // storage.reportStatistics(loadedRecords);

    // // Task 3: Search for rows with attribute “FG_PCT_home” from 0.6 to 0.9 (both inclusively)
    // // Brute-force linear scan
    // storage.bruteForceScan(records, 0.6, 0.9);

    return 0;
}
