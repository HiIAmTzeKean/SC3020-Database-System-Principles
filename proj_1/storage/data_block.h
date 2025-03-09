#ifndef DATA_BLOCK_H
#define DATA_BLOCK_H

#include <cstdint>
#include <ostream>
#include <vector>

struct Record {
  uint32_t game_date_est; // 4 bytes (YYYYMMDD)
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

struct DataBlock {
  int id = -1;
  std::vector<Record> records{};

  DataBlock() {};
  DataBlock(int id, std::istream &buf);
  static int max_records(size_t bytes);
  int serialize(std::ostream &stream) const;
};

std::vector<Record> read_records_from_file(const std::string &filename);

#endif // DATA_BLOCK_H
