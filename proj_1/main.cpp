#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

#include "record.h"

const int BLOCK_SIZE = 4096;

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
}

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
}

void reportStatistics(const std::vector<Record>& records) {
    int recordSize = Record::size();
    int totalRecords = records.size();
    int recordsPerBlock = Block::maxRecordsPerBlock();
    int totalBlocks = (totalRecords + recordsPerBlock - 1) / recordsPerBlock;

    std::cout << "Record size: " << recordSize << " bytes\n";
    std::cout << "Total records: " << totalRecords << "\n";
    std::cout << "Records per block: " << recordsPerBlock << "\n";
    std::cout << "Total blocks needed: " << totalBlocks << "\n";
}

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

    return 0;
}