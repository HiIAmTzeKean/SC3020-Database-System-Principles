#include "data_block.h"
#include "serialize.h"
#include <assert.h>
#include <fstream>
#include <iostream>
#include <istream>
#include <sstream>

int Record::size_unpadded() {
  return sizeof(game_date_est) + sizeof(team_id_home) + sizeof(pts_home) +
         sizeof(fg_pct_home) + sizeof(ft_pct_home) + sizeof(fg3_pct_home) +
         sizeof(ast_home) + sizeof(reb_home) + sizeof(home_team_wins);
}

int Record::size() { return sizeof(Record); }

DataBlock::DataBlock(int id, std::istream &stream) : id(id) {
  while (stream.good()) {
    Record record;
    record.game_date_est = Serializer::read_uint32(stream);
    record.team_id_home = Serializer::read_uint32(stream);
    record.fg_pct_home = Serializer::read_float(stream);
    record.ft_pct_home = Serializer::read_float(stream);
    record.fg3_pct_home = Serializer::read_float(stream);
    record.ast_home = Serializer::read_uint16(stream);
    record.reb_home = Serializer::read_uint16(stream);
    record.pts_home = Serializer::read_uint16(stream);
    record.home_team_wins = Serializer::read_bool(stream);
    if (!stream.fail()) {
      records.push_back(record);
    }
  }
}

int DataBlock::max_records(size_t block_size) {
  return block_size / Record::size();
}

int DataBlock::serialize(std::ostream &stream) const {
  int size = 0;
  for (const auto &record : records) {
    size += Serializer::write_uint32(stream, record.game_date_est);
    size += Serializer::write_uint32(stream, record.team_id_home);
    size += Serializer::write_float(stream, record.fg_pct_home);
    size += Serializer::write_float(stream, record.ft_pct_home);
    size += Serializer::write_float(stream, record.fg3_pct_home);
    size += Serializer::write_uint16(stream, record.ast_home);
    size += Serializer::write_uint16(stream, record.reb_home);
    size += Serializer::write_uint16(stream, record.pts_home);
    size += Serializer::write_bool(stream, record.home_team_wins);
  }
  return size;
}

std::vector<Record> read_records_from_file(const std::string &filename) {
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
    Record record{};
    std::string game_date_est;
    // Skip line if field value is missing
    if (!(ss >> game_date_est >> record.team_id_home >> record.pts_home >>
          record.fg_pct_home >> record.ft_pct_home >> record.fg3_pct_home >>
          record.ast_home >> record.reb_home >> record.home_team_wins)) {
      num_skips++;
      continue;
    }
    // Convert game_date_est string (DD/MM/YYYY) to uint32_t as YYYYMMDD
    int day, month, year;
    if (sscanf(game_date_est.c_str(), "%2d/%2d/%4d", &day, &month, &year) ==
        3) {
      record.game_date_est = (year * 10000) + (month * 100) + day;
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
