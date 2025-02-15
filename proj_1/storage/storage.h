#ifndef STORAGE_H
#define STORAGE_H

#define BLOCK_SIZE 4096

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Record {
    char game_date_est[11];  // "DD/MM/YYY\0" (11 bytes)
    int team_id_home;        // 4 bytes
    int pts_home;            // 4 bytes
    float fg_pct_home;       // 4 bytes
    float ft_pct_home;       // 4 bytes
    float fg3_pct_home;      // 4 bytes
    int ast_home;            // 4 bytes
    int reb_home;            // 4 bytes
    bool home_team_wins;     // 1 byte

    static int size();
};

struct Block {
    std::vector<Record> records;

    static int maxRecordsPerBlock();
    void serialize(char *buffer);
};

struct Storage {
    std::vector<Record> readRecordsFromFile(const std::string &filename);

    void writeDatabaseFile(const std::string &filename, const std::vector<Record> &records);
    void reportStatistics(const std::vector<Record> &records);
};

#endif  // STORAGE_H