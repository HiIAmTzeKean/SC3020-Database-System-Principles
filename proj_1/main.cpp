#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

#include "record.h"
#include "bp_tree.h"

const int BLOCK_SIZE = 4096;

// TODO change the variable and function naming convention LOL
struct Block {
    std::vector<Record> records;
    // assume non-spanning records
    static int maxRecordsPerBlock() {
        return BLOCK_SIZE / Record::size();
    }

    void serialize(char* buffer) {
        int offset = 0;
        for (const auto& record : records) {
            std::memcpy(buffer + offset, &record, Record::size());
            offset += Record::size();
        }
    }
};

std::vector<Record> readRecordsFromFile(const std::string& filename) {
    std::vector<Record> records;
    std::ifstream file(filename);

    if (!file) {
        std::cerr << "Error: Unable to open file " << filename << "\n";
        return records;
    }

    std::string line;
    // Skip the header line
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        Record record;

        std::string game_date;
        ss >> game_date;
        std::strncpy(record.game_date_est, game_date.c_str(), sizeof(record.game_date_est) - 1);

        ss >> record.team_id_home;
        ss >> record.pts_home;
        ss >> record.fg_pct_home;
        ss >> record.ft_pct_home;
        ss >> record.fg3_pct_home;
        ss >> record.ast_home;
        ss >> record.reb_home;
        ss >> record.home_team_wins;

        records.push_back(record);
    }

    file.close();
    return records;
};

void writeDatabaseFile(const std::string& filename, const std::vector<Record>& records) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    int maxRecords = Block::maxRecordsPerBlock();
    char buffer[BLOCK_SIZE];
    Block block;
    int totalBlocks = 0;

    for (const auto& record : records) {
        block.records.push_back(record);
        if (block.records.size() == maxRecords) {
            block.serialize(buffer);
            file.write(buffer, BLOCK_SIZE);
            block.records.clear();
            totalBlocks++;
        }
    }

    if (!block.records.empty()) {
        block.serialize(buffer);
        file.write(buffer, BLOCK_SIZE);
        totalBlocks++;
    }

    file.close();
    std::cout << "Database file written successfully.\n";
    std::cout << "Total blocks written: " << totalBlocks << "\n";
};

void reportStatistics(const std::vector<Record>& records) {
    int recordSize = Record::size();
    int totalRecords = records.size();
    int recordsPerBlock = Block::maxRecordsPerBlock();
    int totalBlocks = (totalRecords + recordsPerBlock - 1) / recordsPerBlock;

    std::cout << "Record size: " << recordSize << " bytes\n";
    std::cout << "Total records: " << totalRecords << "\n";
    std::cout << "Records per block: " << recordsPerBlock << "\n";
    std::cout << "Total blocks needed: " << totalBlocks << "\n";
};

// TODO fix this function corruption during reading
std::vector<Record> readDatabaseFile(const std::string& filename) {
    std::vector<Record> records;
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file for reading.\n";
        return records;
    }

    char buffer[BLOCK_SIZE];
    while (file.read(buffer, BLOCK_SIZE)) {
        Block block;
        int offset = 0;
        while (offset < BLOCK_SIZE) {
            Record record;
            std::memcpy(&record, buffer + offset, Record::size());
            records.push_back(record);
            offset += Record::size();
        }
    }

    file.close();
    return records;
};

int main() {
    std::string inputFile = "games.txt";
    std::vector<Record> records = readRecordsFromFile(inputFile);

    if (records.empty()) {
        std::cerr << "No records found in the file.\n";
        return 1;
    }

    reportStatistics(records);

    std::string outputFile = "nba_database.dat";
    writeDatabaseFile(outputFile, records);

    // Load the binary file and insert records into the B+ tree
    // std::vector<Record> loadedRecords = readDatabaseFile(outputFile);
    BPlusTree bptree = BPlusTree(KEY_SIZE);

    int count = 0;
    for (const auto& record : records) {
        // print the record
        std::cout << record.fg_pct_home << " " << record.ft_pct_home << " " << record.fg3_pct_home << " " << record.ast_home << " " << record.reb_home << " " << record.home_team_wins << "\n";
        bptree.insert(record.fg_pct_home, new Record(record));
        std::cout << "Record " << ++count << " inserted into the B+ tree.\n";
        bptree.print();
        std::cout << "\n";
    }

    std::cout << "Records loaded into the B+ tree successfully.\n";

    return 0;
};