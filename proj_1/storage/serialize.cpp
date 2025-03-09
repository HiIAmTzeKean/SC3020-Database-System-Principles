#include "serialize.h"
#include <cstring>
#include <iostream>
static std::uint8_t buffer[8];

namespace Serializer {
std::uint8_t read_uint8(std::istream &stream) {
  stream.read(reinterpret_cast<char *>(buffer), 1);
  return buffer[0];
}

std::uint16_t read_uint16(std::istream &stream) {
  stream.read(reinterpret_cast<char *>(buffer), 2);
  return ((uint16_t)buffer[0] << 8) | ((uint16_t)buffer[1]);
}

std::uint32_t read_uint32(std::istream &stream) {
  stream.read(reinterpret_cast<char *>(buffer), 4);
  return ((uint32_t)buffer[0] << 24) | ((uint32_t)buffer[1] << 16) |
         ((uint32_t)buffer[2] << 8) | ((uint32_t)buffer[3] << 0);
}

std::uint64_t read_uint64(std::istream &stream) {
  stream.read(reinterpret_cast<char *>(buffer), 8);
  return ((uint64_t)buffer[0] << 56) | ((uint64_t)buffer[1] << 48) |
         ((uint64_t)buffer[2] << 40) | ((uint64_t)buffer[3] << 32) |
         ((uint64_t)buffer[4] << 24) | ((uint64_t)buffer[5] << 16) |
         ((uint64_t)buffer[6] << 8) | ((uint64_t)buffer[7] << 0);
}

float read_float(std::istream &stream) {
  static_assert(sizeof(float) == sizeof(std::uint32_t),
                "Float has unexpected size.");
  auto value = read_uint32(stream);
  float ret;
  std::memcpy(&ret, &value, sizeof(float));
  return ret;
}

double read_double(std::istream &stream) {
  static_assert(sizeof(double) == sizeof(std::uint64_t),
                "Double has unexpected size.");
  auto value = read_uint64(stream);
  double ret;
  std::memcpy(&ret, &value, sizeof(double));
  return ret;
}

bool read_bool(std::istream &stream) { return read_uint8(stream) != 0; }

size_t write_uint8(std::ostream &stream, std::uint8_t x) {
  buffer[0] = x;
  stream.write(reinterpret_cast<char *>(buffer), 1);
  return 1;
};

size_t write_uint16(std::ostream &stream, std::uint16_t x) {
  buffer[0] = (x >> 8) & 0xFF;
  buffer[1] = (x >> 0) & 0xFF;
  stream.write(reinterpret_cast<char *>(buffer), 2);
  return 2;
};

size_t write_uint32(std::ostream &stream, std::uint32_t x) {
  buffer[0] = (x >> 24) & 0xFF;
  buffer[1] = (x >> 16) & 0xFF;
  buffer[2] = (x >> 8) & 0xFF;
  buffer[3] = (x >> 0) & 0xFF;
  stream.write(reinterpret_cast<char *>(buffer), 4);
  return 4;
};

size_t write_uint64(std::ostream &stream, std::uint64_t x) {
  buffer[0] = (x >> 56) & 0xFF;
  buffer[1] = (x >> 48) & 0xFF;
  buffer[2] = (x >> 40) & 0xFF;
  buffer[3] = (x >> 32) & 0xFF;
  buffer[4] = (x >> 24) & 0xFF;
  buffer[5] = (x >> 16) & 0xFF;
  buffer[6] = (x >> 8) & 0xFF;
  buffer[7] = (x >> 0) & 0xFF;
  stream.write(reinterpret_cast<char *>(buffer), 8);
  return 8;
};

size_t write_float(std::ostream &stream, float x) {
  std::uint32_t write;
  std::memcpy(&write, &x, sizeof(float));
  return write_uint32(stream, write);
}

size_t write_double(std::ostream &stream, double x) {
  std::uint64_t write;
  std::memcpy(&write, &x, sizeof(double));
  return write_uint64(stream, write);
}

size_t write_bool(std::ostream &stream, bool x) {
  return write_uint8(stream, x);
}

}; // namespace Serializer
