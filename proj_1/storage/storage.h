#ifndef STORAGE_H
#define STORAGE_H

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#else
#include <cstdio>
#endif

struct Record {
  uint32_t game_date_est; // 4 bytes (DDMMYYYY)
  uint32_t team_id_home;  // 4 bytes (0-4,294,967,295)
  float fg_pct_home;      // 4 bytes
  float ft_pct_home;      // 4 bytes
  float fg3_pct_home;     // 4 bytes
  uint16_t ast_home;      // 2 bytes (0-65,535)
  uint16_t reb_home;      // 2 bytes (0-65,535)
  uint16_t pts_home;      // 2 bytes (0-65,535)
  bool home_team_wins;    // 1 byte

  static int size_unpadded(); // 27 bytes
  static int size();          // 28 bytes (actual size w/ padding)
};

struct Block {
  std::vector<Record> records;
  uint16_t id = 0;

  int serialize(char *buffer);

  void deserialize(char *buffer, int bytesToRead);
};

struct LoadedBlock {
  int id;
  Block block;
};

class Storage {
public:
  int number_of_records = 0;
  std::vector<LoadedBlock> loaded_blocks;

  int block_size;
  int max_records_per_block();

  Storage() : block_size(get_system_block_size_setting()) {
    m_buffer = new char[block_size]{};
  };
  ~Storage() { delete[] m_buffer; };

  int write_database_file(const std::string &filename,
                          const std::vector<Record> &records);

  Block *read_block(int id);
  Block read_database_file(const std::string &filename);
  std::vector<Record> read_records_from_file(const std::string &filename);

  void report_statistics();
  void brute_force_scan(std::vector<Record> const &records, float min,
                        float max);

private:
  int get_system_block_size_setting(void);
  char *m_buffer;
};

#endif // STORAGE_H
