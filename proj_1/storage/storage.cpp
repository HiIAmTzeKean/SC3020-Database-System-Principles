#include "storage.h"
#include <assert.h>

int Storage::BlockSize = 0;

int Record::sizeUnpadded() {
  return sizeof(game_date_est) + sizeof(team_id_home) + sizeof(pts_home) +
         sizeof(fg_pct_home) + sizeof(ft_pct_home) + sizeof(fg3_pct_home) +
         sizeof(ast_home) + sizeof(reb_home) + sizeof(home_team_wins);
}

// Actual size allocated with padding
int Record::size() { return sizeof(Record); }

int Block::maxRecordsPerBlock() { return Storage::BlockSize / Record::size(); }

// Serialize Block data into a buffer
void Block::serialize(char *buffer, int *bytesToWrite) {
  int offset = 0;

  // Serialize the block id
  std::memcpy(buffer + offset, &id, sizeof(id));
  offset += sizeof(id);

  // Serialize the records
  for (const auto &record : records) {
    std::memcpy(buffer + offset, &record, Record::size());
    offset += Record::size();
  }
  *bytesToWrite = offset;
}

void Block::deserialize(char *buffer, int bytesToRead) {
  int offset = 0;
  // Deserialize the block id
  std::memcpy(&id, buffer + offset, sizeof(id));
  offset += sizeof(id);

  // Deserialize the records
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
    if (!(ss >> game_date_est >> record.team_id_home >> record.pts_home >>
          record.fg_pct_home >> record.ft_pct_home >> record.fg3_pct_home >>
          record.ast_home >> record.reb_home >> record.home_team_wins)) {
      numSkips++;
      continue;
    }

    // Convert game_date_est string (DD/MM/YYYY) to uint32_t as DDMMYYYY
    int day, month, year;
    if (sscanf(game_date_est.c_str(), "%2d/%2d/%4d", &day, &month, &year) ==
        3) {
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

int Storage::writeDatabaseFile(const std::string &filename,
                               const std::vector<Record> &records) {
  int maxRecords = Block::maxRecordsPerBlock();
  char buffer[BlockSize];
  Block block;
  int bytesToWrite;
  int totalBlocks = 0;
  int totalRecords = 0;

  for (const auto &record : records) {
    block.records.push_back(record);
    if (block.records.size() == maxRecords) {
      block.id = totalBlocks;
      block.serialize(buffer, &bytesToWrite);
      std::string blockFilename = filename + std::to_string(block.id) + ".dat";
      std::ofstream file(blockFilename, std::ios::binary);
      if (!file) {
        std::cerr << "Error opening file for writing.\n";
        return -1;
      }
      file.write(buffer, BlockSize);
      file.close();
      totalBlocks++;
      totalRecords += block.records.size();
      block.records.clear();
    }
  }

  // Serialize partial block
  if (!block.records.empty()) {
    block.id = totalBlocks;
    block.serialize(buffer, &bytesToWrite);
    std::string blockFilename = filename + std::to_string(block.id) + ".dat";
    std::ofstream file(blockFilename, std::ios::binary);
    if (!file) {
      std::cerr << "Error opening file for writing.\n";
      return -1;
    }
    file.write(buffer,
               bytesToWrite); // Write only the used portion of the block
    file.close();
    totalBlocks++;
    totalRecords += block.records.size();
    block.records.clear();
  }

  std::cout << "Database file written successfully.\n";
  std::cout << "Total blocks written: " << totalBlocks << "\n";
  std::cout << "Total records written: " << totalRecords << "\n";
  return totalBlocks;
}

Block Storage::readDatabaseFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Error opening file for reading.");
  }

  char buffer[BlockSize];
  Block block;

  while (file.read(buffer, BlockSize) || file.gcount()) {
    int bytesToRead = file.gcount() < BlockSize
                          ? file.gcount() // Read partial block
                          : BlockSize;
    block.deserialize(buffer, bytesToRead);
  }
  number_of_records += block.records.size();
  loaded_blocks.push_back({.id = block.id, .block = block});
  file.close();
  return block;
}

void Storage::reportStatistics() {
  int recordsPerBlock = Block::maxRecordsPerBlock();

  std::cout << "\nTask 1: Storage" << "\n";
  std::cout << "Record size: " << Record::size() << " bytes ("
            << Record::sizeUnpadded() << " bytes without padding)\n";
  std::cout << "Number of Records: " << number_of_records << "\n";
  std::cout << "Number of Records per Block: " << recordsPerBlock << "\n";
  std::cout << "Number of Blocks: " << loaded_blocks.size() << "\n";
}

Block *Storage::read_block(int id) {
  for (LoadedBlock &block : this->loaded_blocks) {
    if (id == block.id)
      return &block.block;
  }
  // This block is currently not loaded.
  readDatabaseFile("data/block_" + std::to_string(id) + ".dat");
  auto latest_load = this->loaded_blocks.rbegin();
  assert(latest_load->id == id);
  return &latest_load->block;
}

void Storage::bruteForceScan(std::vector<Record> const &records, float min,
                             float max) {
  int blockCount = 0;
  int filteredRecordCount = 0;
  float sum = 0;

  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < records.size(); i++) {
    if (i % Block::maxRecordsPerBlock() == 0) {
      blockCount++;
    }

    if (records[i].fg_pct_home >= min && records[i].fg_pct_home <= max) {
      sum += records[i].fg_pct_home;
      filteredRecordCount++;
    }
  }
  auto end = std::chrono::high_resolution_clock::now(); // End time

  std::cout << "\nTask 3: Brute-force Linear Scan (search 'FG_PCT_HOME' from "
               "0.6 to 0.9, both inclusively)"
            << "\n";
  std::cout << "Number of records found in range: " << filteredRecordCount
            << '\n';
  std::cout << "Number of data blocks accessed: " << blockCount << '\n';
  if (filteredRecordCount > 0) {
    float avg = sum / filteredRecordCount;
    std::cout << "Average of 'FG_PCT_home': " << avg << '\n';
  }

  std::chrono::duration<double> time_taken = end - start; // Duration in seconds
  std::cout << "Brute-force scan time: " << time_taken.count() << " seconds"
            << '\n';
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
    std::cout << "Failed to get system block size, setting block size to "
                 "default of 4096 bytes.\n";
  }
#else
  block_size = 4096;
  std::cout << "Failed to get system block size, setting block size to default "
               "of 4096 bytes.\n";
#endif

  std::cout << "System block size: " << block_size << "bytes \n";
  return block_size;
}
