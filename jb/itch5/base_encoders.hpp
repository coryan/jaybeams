#ifndef jb_itch5_base_encoders_hpp
#define jb_itch5_base_encoders_hpp

#include <jb/itch5/encoder.hpp>

#include <cstdint>

namespace jb {
namespace itch5 {

/// Specialize jb::itch5::encoder for 1-byte integer (or character) fields.
template <bool validate> struct encoder<validate, std::uint8_t> {
  static void w(std::size_t size, void* msg, std::size_t offset,
                std::uint8_t x) {
    check_offset<validate>("encode std::uint8_t", size, offset, 1);
    static_cast<std::uint8_t*>(msg)[offset] = x;
  }
};

/// Specialize jb::itch5::encoder for 2-byte integer fields.
template <bool validate> struct encoder<validate, std::uint16_t> {
  /// Please see the generic documentation for jb::itch5::encoder<>::w()
  static void w(std::size_t size, void* msg, std::size_t offset,
                std::uint16_t x) {
    check_offset<validate>("encode std::uint16_t", size, offset, 2);
    encoder<false, std::uint8_t>::w(size, msg, offset, std::uint8_t(x >> 8));
    encoder<false, std::uint8_t>::w(size, msg, offset + 1,
                                    std::uint8_t(x & 0xff));
  }
};

/// Specialize jb::itch5::encoder for 4-byte integer fields.
template <bool validate> struct encoder<validate, std::uint32_t> {
  /// Please see the generic documentation for jb::itch5::encoder<>::w()
  static void w(std::size_t size, void* msg, std::size_t offset,
                std::uint32_t x) {
    check_offset<validate>("encode std::uint32_t", size, offset, 4);
    encoder<false, std::uint16_t>::w(size, msg, offset, std::uint16_t(x >> 16));
    encoder<false, std::uint16_t>::w(size, msg, offset + 2,
                                     std::uint16_t(x & 0xffff));
  }
};

/// Specialize jb::itch5::encoder for 4-byte integer fields.
template <bool validate> struct encoder<validate, std::uint64_t> {
  /// Please see the generic documentation for jb::itch5::encoder<>::w()
  static void w(std::size_t size, void* msg, std::size_t offset,
                std::uint64_t x) {
    check_offset<validate>("encode std::uint32_t", size, offset, 8);
    encoder<false, std::uint32_t>::w(size, msg, offset, std::uint32_t(x >> 32));
    encoder<false, std::uint32_t>::w(size, msg, offset + 4,
                                     std::uint32_t(x & 0xffffffff));
  }
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_base_encoders_hpp
