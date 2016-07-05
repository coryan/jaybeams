#ifndef jb_itch5_base_decoders_hpp
#define jb_itch5_base_decoders_hpp

#include <jb/itch5/decoder.hpp>
#include <cstdint>

namespace jb {
namespace itch5 {

/// Specialize jb::itch5::decoder for 1-byte integer (or character) fields.
template<bool validate>
struct decoder<validate,std::uint8_t> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static std::uint8_t r(std::size_t size, void const* msg, std::size_t offset) {
    check_offset<validate>("std::uint8_t", size, offset, 1);
    return static_cast<std::uint8_t const*>(msg)[offset];
  }
};

/// Specialize jb::itch5::decoder for 2-byte integer fields.
template<bool validate>
struct decoder<validate,std::uint16_t> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static std::uint16_t r(
      std::size_t size, void const* msg, std::size_t offset) {
    check_offset<validate>("std::uint16_t", size, offset, 2);
    std::uint8_t hi = decoder<false,std::uint8_t>::r(size, msg, offset);
    std::uint8_t lo = decoder<false,std::uint8_t>::r(size, msg, offset + 1);
    return std::uint16_t(hi) << 8 | lo;
  }
};

/// Specialize jb::itch5::decoder for 4-byte integer fields.
template<bool validate>
struct decoder<validate,std::uint32_t> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static std::uint32_t r(
      std::size_t size, void const* msg, std::size_t offset) {
    check_offset<validate>("std::uint32_t", size, offset, 4);
    std::uint16_t hi = decoder<false,std::uint16_t>::r(size, msg, offset);
    std::uint16_t lo = decoder<false,std::uint16_t>::r(size, msg, offset + 2);
    return std::uint32_t(hi) << 16 | lo;
  }
};

/// Specialize jb::itch5::decoder for 4-byte integer fields.
template<bool validate>
struct decoder<validate,std::uint64_t> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static std::uint64_t r(
      std::size_t size, void const* msg, std::size_t offset) {
    check_offset<validate>("std::uint32_t", size, offset, 8);
    std::uint32_t hi = decoder<false,std::uint32_t>::r(size, msg, offset);
    std::uint32_t lo = decoder<false,std::uint32_t>::r(size, msg, offset + 4);
    return std::uint64_t(hi) << 32 | lo;
  }
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_base_decoders_hpp
