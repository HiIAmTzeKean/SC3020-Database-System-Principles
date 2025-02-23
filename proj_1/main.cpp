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

    // std::string outputFile = "nba_database.dat";
    // storage.writeDatabaseFile(outputFile, records);

    // std::vector<Record> loadedRecords;
    // storage.readDatabaseFile(outputFile, loadedRecords);

    // // Task 1: Storage component
    // storage.reportStatistics(loadedRecords);

    // // Task 3: Search for rows with attribute “FG_PCT_home” from 0.6 to 0.9 (both inclusively)
    // // Brute-force linear scan
    // storage.bruteForceScan(records, 0.6, 0.9);

    int num_records = 0;
    BPlusTree tree = BPlusTree(5);
    for (Record &record : records) {
        tree.insert(record.fg_pct_home, &record);
        num_records++;
    };
    std::cout << "Number of records: " << num_records << std::endl;

    // int number_of_target = 0;
    // std::vector<Record*> searchResults = tree.search(0.422);
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
