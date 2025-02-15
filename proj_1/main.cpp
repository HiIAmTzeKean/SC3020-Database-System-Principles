#include "storage/storage.h"

int main() {
    std::string inputFile = "games.txt";
    Storage storage;
    std::vector<Record> records = storage.readRecordsFromFile(inputFile);

    if (records.empty()) {
        std::cerr << "No records found in the file.\n";
        return 1;
    }

    storage.reportStatistics(records);

    std::string outputFile = "nba_database.dat";
    storage.writeDatabaseFile(outputFile, records);

    return 0;
}