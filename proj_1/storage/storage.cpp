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
void Block::serialize(char *buffer, int *bytesToWrite) {
    int offset = 0;
    int recordSize = 0;
    for (const auto &record : records) {
        std::memcpy(buffer + offset, &record, Record::size());
        offset += Record::size();
    }
    *bytesToWrite = offset;
}

void Block::deserialize(char *buffer, std::vector<Record> &records, int bytesToRead) {
    int offset = 0;
    while (offset + Record::size() <= bytesToRead) {
        Record record;
        std::memcpy(&record, buffer + offset, Record::size());
        records.push_back(record);
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
        // Skip line if field value is missing
        if (!(ss >> game_date >> 
              record.team_id_home >> 
              record.pts_home >> 
              record.fg_pct_home >> 
              record.ft_pct_home >> 
              record.fg3_pct_home >> 
              record.ast_home >> 
              record.reb_home >> 
              record.home_team_wins)) {
            continue;
        }

        std::strncpy(record.game_date_est, game_date.c_str(), sizeof(record.game_date_est) - 1);
        record.game_date_est[sizeof(record.game_date_est) - 1] = '\0';
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
    int bytesToWrite;
    int totalBlocks = 0;
    int totalRecords = 0;

    for (const auto &record : records) {
        block.records.push_back(record);
        if (block.records.size() == maxRecords) {
            block.serialize(buffer, &bytesToWrite);
            file.write(buffer, BLOCK_SIZE);
            totalBlocks++;
            totalRecords += block.records.size();
            block.records.clear();
        }
    }

    // Serialize partial block
    if (!block.records.empty()) {
        block.serialize(buffer, &bytesToWrite);
        file.write(buffer, bytesToWrite);  // Write only the used portion of the block
        totalBlocks++;
        totalRecords += block.records.size();
        block.records.clear();
    }
    file.close();

    std::cout << "Database file written successfully.\n";
    std::cout << "Total blocks written: " << totalBlocks << "\n";
    std::cout << "Total records written: " << totalRecords << "\n";
}

void Storage::readDatabaseFile(const std::string &filename, std::vector<Record> &records) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file for reading.\n";
        return;
    }

    char buffer[BLOCK_SIZE];
    Block block;
    int blockCount = 0;
    int recordCount = 0;

    while (file.read(buffer, BLOCK_SIZE) || file.gcount()) {
        int bytesToRead = file.gcount() < BLOCK_SIZE 
            ? file.gcount() // Read partial block 
            : BLOCK_SIZE;

        block.deserialize(buffer, records, bytesToRead);
        blockCount++;
    }
    file.close();

    std::cout << "Database file read successfully.\n";
    std::cout << "Total blocks read: " << blockCount << "\n";
    std::cout << "Total records read: " << records.size() << "\n";
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