#include "storage.h"
#include <assert.h>

int Record::size_unpadded() {
  return sizeof(game_date_est) + sizeof(team_id_home) + sizeof(pts_home) +
         sizeof(fg_pct_home) + sizeof(ft_pct_home) + sizeof(fg3_pct_home) +
         sizeof(ast_home) + sizeof(reb_home) + sizeof(home_team_wins);
}

// Actual size allocated with padding
int Record::size() { return sizeof(Record); }

int Storage::max_records_per_block() { return block_size / Record::size(); }

// Serialize Block data into a buffer
int Block::serialize(char *buffer) {
  // Serialize the block id
  std::memcpy(buffer, &id, sizeof(id));
  int offset = sizeof(id);

  // Serialize the records
  for (const auto &record : records) {
    std::memcpy(buffer + offset, &record, Record::size());
    offset += Record::size();
  }
  return offset;
}

void Block::deserialize(char *buffer, int bytes_to_read) {
  int offset = 0;
  // Deserialize the block id
  std::memcpy(&id, buffer + offset, sizeof(id));
  offset += sizeof(id);

  // Deserialize the records
  while (offset + Record::size() <= bytes_to_read) {
    Record record;
    std::memcpy(&record, buffer + offset, Record::size());
    records.push_back(record);
    offset += Record::size();
  }
}

std::vector<Record>
Storage::read_records_from_file(const std::string &filename) {
  std::vector<Record> records;
  std::ifstream file(filename);

  if (!file) {
    std::cerr << "Error: Unable to open file " << filename << std::endl;
    return records;
  }

  std::string line;
  // Skip the header line
  std::getline(file, line);

  int num_skips = 0;

  while (std::getline(file, line)) {
    std::istringstream ss(line);
    Record record;

    std::memset(&record, 0, sizeof(Record));
    std::string game_date_est;
    // Skip line if field value is missing
    if (!(ss >> game_date_est >> record.team_id_home >> record.pts_home >>
          record.fg_pct_home >> record.ft_pct_home >> record.fg3_pct_home >>
          record.ast_home >> record.reb_home >> record.home_team_wins)) {
      num_skips++;
      continue;
    }

    // Convert game_date_est string (DD/MM/YYYY) to uint32_t as DDMMYYYY
    int day, month, year;
    if (sscanf(game_date_est.c_str(), "%2d/%2d/%4d", &day, &month, &year) ==
        3) {
      record.game_date_est = (day * 1000000) + (month * 10000) + year;
    } else {
      std::cerr << "Error: Invalid date format: " << game_date_est << std::endl;
      continue;
    }
    records.push_back(record);
  }

  std::cout << "Number of skipped records: " << num_skips << std::endl;
  file.close();
  return records;
}

int Storage::write_database_file(const std::string &filename,
                                 const std::vector<Record> &records) {
  int max_records = max_records_per_block();
  Block block;
  int total_blocks = 0;
  int total_records = 0;

  for (const auto &record : records) {
    block.records.push_back(record);
    if (block.records.size() == max_records) {
      block.id = total_blocks;
      int bytes_to_write = block.serialize(m_buffer);
      std::string block_filename = filename + std::to_string(block.id) + ".dat";
      std::ofstream file(block_filename, std::ios::binary);
      if (!file) {
        std::cerr << "Error opening file for writing." << std::endl;
        return -1;
      }
      file.write(m_buffer, bytes_to_write);
      file.close();
      total_blocks++;
      total_records += block.records.size();
      block.records.clear();
    }
  }

  // Serialize partial block
  if (!block.records.empty()) {
    block.id = total_blocks;
    int bytes_to_write = block.serialize(m_buffer);
    std::string block_filename = filename + std::to_string(block.id) + ".dat";
    std::ofstream file(block_filename, std::ios::binary);
    if (!file) {
      std::cerr << "Error opening file for writing." << std::endl;
      return -1;
    }
    file.write(m_buffer,
               bytes_to_write); // Write only the used portion of the block
    file.close();
    total_blocks++;
    total_records += block.records.size();
    block.records.clear();
  }

  std::cout << "Database file written successfully." << std::endl;
  std::cout << "Total blocks written: " << total_blocks << std::endl;
  std::cout << "Total records written: " << total_records << std::endl;

  return total_blocks;
}

Block Storage::read_database_file(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Error opening file for reading.");
  }

  Block block;

  while (file.read(m_buffer, block_size) || file.gcount()) {
    int bytes_to_read = file.gcount() < block_size
                            ? file.gcount() // Read partial block
                            : block_size;
    block.deserialize(m_buffer, bytes_to_read);
  }
  number_of_records += block.records.size();
  loaded_blocks.push_back({.id = block.id, .block = block});
  file.close();
  return block;
}

void Storage::report_statistics() {
  int records_per_block = max_records_per_block();

  std::cout << "Task 1: Storage" << std::endl;
  std::cout << "Record size: " << Record::size() << " bytes ("
            << Record::size_unpadded() << " bytes without padding)"
            << std::endl;
  std::cout << "Number of Records: " << number_of_records << std::endl;
  std::cout << "Number of Records per Block: " << records_per_block
            << std::endl;
  std::cout << "Number of Blocks: " << loaded_blocks.size() << std::endl;
}

Block *Storage::read_block(int id) {
  for (LoadedBlock &block : this->loaded_blocks) {
    if (id == block.id)
      return &block.block;
  }
  // This block is currently not loaded.
  read_database_file("data/block_" + std::to_string(id) + ".dat");
  auto latest_load = this->loaded_blocks.rbegin();
  assert(latest_load->id == id);
  return &latest_load->block;
}

void Storage::brute_force_scan(std::vector<Record> const &records, float min,
                               float max) {
  int block_count = 0;
  int filtered_record_count = 0;
  float sum = 0;

  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < records.size(); i++) {
    if (i % max_records_per_block() == 0) {
      block_count++;
    }

    if (records[i].fg_pct_home >= min && records[i].fg_pct_home <= max) {
      sum += records[i].fg_pct_home;
      filtered_record_count++;
    }
  }
  auto end = std::chrono::high_resolution_clock::now(); // End time

  std::cout << std::endl
            << "Task 3: Brute-force Linear Scan (search 'FG_PCT_HOME' from "
               "0.6 to 0.9, both inclusively)"
            << std::endl;
  std::cout << "Number of records found in range: " << filtered_record_count
            << std::endl;
  std::cout << "Number of data blocks accessed: " << block_count << std::endl;
  if (filtered_record_count > 0) {
    float avg = sum / filtered_record_count;
    std::cout << "Average of 'FG_PCT_home': " << avg << std::endl;
  }

  std::chrono::duration<double> time_taken = end - start; // Duration in seconds
  std::cout << "Brute-force scan time: " << time_taken.count() << " seconds"
            << std::endl;
}

// Get the current system's block size (page size) in bytes
int Storage::get_system_block_size_setting(void) {
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
                 "default of 4096 bytes."
              << std::endl;
  }
#else
  block_size = 4096;
  std::cout << "Failed to get system block size, setting block size to default "
               "of 4096 bytes."
            << std::endl;
#endif

  std::cout << "System block size: " << block_size << " byte" << std::endl;
  return block_size;
}
