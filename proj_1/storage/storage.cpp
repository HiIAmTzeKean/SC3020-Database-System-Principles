#include "storage.h"

int Record::size() {
    return sizeof(game_date_est) +
           sizeof(team_id_home) +
           sizeof(pts_home) +
           sizeof(fg_pct_home) +
           sizeof(ft_pct_home) +
           sizeof(fg3_pct_home) +
           sizeof(ast_home) +
           sizeof(reb_home) +
           sizeof(home_team_wins);
}

int Block::maxRecordsPerBlock() {
    return BLOCK_SIZE / Record::size();
}

// Serialize Block data into a buffer
void Block::serialize(char *buffer) {
    int offset = 0;
    for (const auto &record : records) {
        std::memcpy(buffer + offset, &record, Record::size());
        offset += Record::size();
    }
}

std::vector<Record> Storage::readRecordsFromFile(const std::string &filename) {
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

void Storage::writeDatabaseFile(const std::string &filename, const std::vector<Record> &records) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file for writing.\n";
        return;
    }

    int maxRecords = Block::maxRecordsPerBlock();
    char buffer[BLOCK_SIZE];
    Block block;
    int totalBlocks = 0;

    for (const auto &record : records) {
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

void Storage::reportStatistics(const std::vector<Record> &records) {
    int recordSize = Record::size();
    int totalRecords = records.size();
    int recordsPerBlock = Block::maxRecordsPerBlock();
    int totalBlocks = (totalRecords + recordsPerBlock - 1) / recordsPerBlock;

    // std::cout << "game_date_est: " << sizeof(Record::game_date_est) << " bytes\n";
    // std::cout << "team_id_home: " << sizeof(Record::team_id_home) << " bytes\n";
    // std::cout << "pts_home: " << sizeof(Record::pts_home) << " bytes\n";
    // std::cout << "fg_pct_home: " << sizeof(Record::fg_pct_home) << " bytes\n";
    // std::cout << "ft_pct_home: " << sizeof(Record::ft_pct_home) << " bytes\n";
    // std::cout << "fg3_pct_home: " << sizeof(Record::fg3_pct_home) << " bytes\n";
    // std::cout << "ast_home: " << sizeof(Record::ast_home) << " bytes\n";
    // std::cout << "reb_home: " << sizeof(Record::reb_home) << " bytes\n";
    // std::cout << "home_team_wins: " << sizeof(Record::home_team_wins) << " bytes\n";
    // std::cout << "Record size: " << sizeof(Record) << " bytes\n";

    std::cout << "Record size: " << recordSize << " bytes\n";
}