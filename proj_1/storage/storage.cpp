#include "storage.h"

int Storage::BlockSize = 0;

int Record::sizeUnpadded() {
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

// Actual size allocated with padding
int Record::size() {
    return sizeof(Record);
}

int Block::maxRecordsPerBlock() {
    return Storage::BlockSize / Record::size();
}

// Serialize Block data into a buffer
void Block::serialize(char *buffer, int *bytesToWrite) {
    int offset = 0;
    for (const auto &record: records) {
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

    int numSkips = 0;

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        Record record;

        std::string game_date_est;
        // Skip line if field value is missing
        if (!(ss >> game_date_est >>
              record.team_id_home >>
              record.pts_home >>
              record.fg_pct_home >>
              record.ft_pct_home >>
              record.fg3_pct_home >>
              record.ast_home >>
              record.reb_home >>
              record.home_team_wins)) {
            numSkips++;
            continue;
        }

        // Convert game_date_est string (DD/MM/YYYY) to uint32_t as DDMMYYYY
        int day, month, year;
        if (sscanf(game_date_est.c_str(), "%2d/%2d/%4d", &day, &month, &year) == 3) {
            record.game_date_est = (day * 1000000) + (month * 10000) + year;
        } else {
            std::cerr << "Error: Invalid date format: " << game_date_est << "\n";
            continue;
        }
        records.push_back(record);
    }

    std::cout << "Number of skipped records: " << numSkips << "\n";
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
    char buffer[BlockSize];
    Block block;
    int bytesToWrite;
    int totalBlocks = 0;
    int totalRecords = 0;

    for (const auto &record: records) {
        block.records.push_back(record);
        if (block.records.size() == maxRecords) {
            block.serialize(buffer, &bytesToWrite);
            file.write(buffer, BlockSize);
            totalBlocks++;
            totalRecords += block.records.size();
            block.records.clear();
        }
    }

    // Serialize partial block
    if (!block.records.empty()) {
        block.serialize(buffer, &bytesToWrite);
        file.write(buffer, bytesToWrite); // Write only the used portion of the block
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

    char buffer[BlockSize];
    Block block;
    int blockCount = 0;

    while (file.read(buffer, BlockSize) || file.gcount()) {
        int bytesToRead = file.gcount() < BlockSize
                              ? file.gcount() // Read partial block
                              : BlockSize;

        block.deserialize(buffer, records, bytesToRead);
        blockCount++;
    }

    //// TODO: remove after done with testing. outputs a .txt file that can be diff-ed with original "games.txt" 
    // std::ofstream debugFile("debug_games.txt");
    // for (auto& record : records) {
    //     uint32_t date = record.game_date_est;
    //     int day = date / 1000000;
    //     int month = (date / 10000) % 100;
    //     int year = date % 10000; 
    //     char game_date_est[11];  // "DD/MM/YYYY\0"
    //     snprintf(game_date_est, sizeof(game_date_est), "%d/%d/%d", day, month, year);

    //     debugFile << game_date_est << "\t"
    //              << record.team_id_home << "\t"
    //              << record.pts_home << "\t"
    //              << record.fg_pct_home << "\t"
    //              << record.ft_pct_home << "\t"
    //              << record.fg3_pct_home << "\t"
    //              << record.ast_home << "\t"
    //              << record.reb_home << "\t"
    //              << record.home_team_wins << "\n";
    // }
    // debugFile.close();
    file.close();

    std::cout << "Database file read successfully.\n";
    std::cout << "Total blocks read: " << blockCount << "\n";
    std::cout << "Total records read: " << records.size() << "\n";
}

void Storage::reportStatistics(const std::vector<Record> &records) {
    int totalRecords = records.size();
    int recordsPerBlock = Block::maxRecordsPerBlock();
    int totalBlocks = (totalRecords + recordsPerBlock - 1) / recordsPerBlock;


    std::cout << "\nTask 1: Storage" << "\n";
    std::cout << "Record size: " << Record::size() << " bytes (" << Record::sizeUnpadded() <<
            " bytes without padding)\n";
    std::cout << "Number of Records: " << totalRecords << "\n";
    std::cout << "Number of Records per Block: " << recordsPerBlock << "\n";
    std::cout << "Number of Blocks: " << totalBlocks << "\n";
}

void Storage::bruteForceScan(std::vector<Record> const &records, float min, float max) {
    int blockCount = 0;
    int filteredRecordCount = 0;
    float sum = 0;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < records.size(); i++) {
        if (i % Block::maxRecordsPerBlock() == 0) {
            blockCount++;
        }

        if (records[i].fg_pct_home >= min && records[i].fg_pct_home <= max) {
            sum += records[i].fg3_pct_home;
            filteredRecordCount++;
        }
    }
    auto end = std::chrono::high_resolution_clock::now(); // End time

    std::cout << "\nTask 3: Brute-force Linear Scan (search 'FG_PCT_HOME' from 0.6 to 0.9, both inclusively)" << "\n";
    std::cout << "Number of records found in range: " << filteredRecordCount << '\n';
    std::cout << "Number of data blocks accessed: " << blockCount << '\n';
    if (filteredRecordCount > 0) {
        float avg = sum / filteredRecordCount;
        std::cout << "Average of 'FG3_PCT_home': " << avg << '\n';
    }

    std::chrono::duration<double> time_taken = end - start; // Duration in seconds
    std::cout << "Brute-force scan time: " << time_taken.count() << " seconds" << '\n';
}

// Get the current system's block size (page size) in bytes
int Storage::getSystemBlockSizeSetting(void) {
    int block_size;
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    block_size = si.dwPageSize;
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
        long res = sysconf(_SC_PAGESIZE);
        block_size = res;
        if (block_size == -1) {
            block_size = 4096;
            std::cout << "Failed to get system block size, setting block size to default of 4096 bytes.\n";
        }
#else
        block_size = 4096;
        std::cout << "Failed to get system block size, setting block size to default of 4096 bytes.\n";
#endif

    std::cout << "System block size: " << block_size << "bytes \n";
    return block_size;
}
