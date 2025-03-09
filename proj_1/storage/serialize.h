#include <cmath>
#include <cstdint>
#include <ios>

namespace Serializer {
std::uint8_t read_uint8(std::istream &);
std::uint16_t read_uint16(std::istream &);
std::uint32_t read_uint32(std::istream &);
std::uint64_t read_uint64(std::istream &);
float read_float(std::istream &);
double read_double(std::istream &);
bool read_bool(std::istream &);

size_t write_uint8(std::ostream &, std::uint8_t);
size_t write_uint16(std::ostream &, std::uint16_t);
size_t write_uint32(std::ostream &, std::uint32_t);
size_t write_uint64(std::ostream &, std::uint64_t);
size_t write_float(std::ostream &, float);
size_t write_double(std::ostream &, double);
size_t write_bool(std::ostream &, bool);

}; // namespace Serializer
