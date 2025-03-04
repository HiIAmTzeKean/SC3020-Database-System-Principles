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

  static int sizeUnpadded(); // 27 bytes
  static int size();         // 28 bytes (actual size w/ padding)
};

struct Block {
  std::vector<Record> records;
  uint16_t id = 0;

  static int maxRecordsPerBlock();

  void serialize(char *buffer, int *bytesToWrite);

  void deserialize(char *buffer, int bytesToRead);
};

struct Storage {
  static int BlockSize;
  int number_of_records = 0;
  std::vector<int> loaded_blocks;
  std::vector<Block> blocks;

  Storage() { BlockSize = getSystemBlockSizeSetting(); };

  std::vector<Record> readRecordsFromFile(const std::string &filename);

  int writeDatabaseFile(const std::string &filename,
                        const std::vector<Record> &records);

  Block readDatabaseFile(const std::string &filename);

  void reportStatistics();

  void bruteForceScan(std::vector<Record> const &records, float min, float max);

  int getSystemBlockSizeSetting(void);
};

#endif // STORAGE_H
