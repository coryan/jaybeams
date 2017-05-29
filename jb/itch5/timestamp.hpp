#ifndef jb_itch5_timestamp_hpp
#define jb_itch5_timestamp_hpp

#include <jb/itch5/base_decoders.hpp>
#include <jb/itch5/base_encoders.hpp>
#include <chrono>
#include <iosfwd>

namespace jb {
namespace itch5 {

/**
 * Represent a ITCH-5.0 timestamp
 *
 * ITCH-5.0 uses nanoseconds since midnight for its timestamps.
 */
struct timestamp {
  std::chrono::nanoseconds ts;
};

/**
 * Validate the a timestamp value.
 *
 * In ITCH-5.0 messages the timestamp represents nanoseconds since
 * midnight.  The protocol is designed to start new sessions at the
 * beginning of each day, so timestamps cannot ever be more than 24
 * hours in nanoseconds.
 *
 * @tparam validate unless it is true the function is a no-op.
 *
 * @throws std::runtime_error if @a tparam is true and the timestamp
 * is out of the expected range.
 */
template <bool validate>
void check_timestamp_range(timestamp const& t) {
}

/**
 * Provide an active implementation of
 * jb::itch5::check_timestamp_range<>
 */
template <>
void check_timestamp_range<true>(timestamp const& t);

/// Specialize jb::itch5::decoder<> for a timestamp
template <bool validate>
struct decoder<validate, timestamp> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static timestamp r(std::size_t size, void const* buf, std::size_t offset) {
    check_offset<validate>("timestamp", size, offset, 6);
    std::uint64_t hi = decoder<false, std::uint16_t>::r(size, buf, offset);
    std::uint64_t lo = decoder<false, std::uint32_t>::r(size, buf, offset + 2);
    timestamp tmp{std::chrono::nanoseconds(hi << 32 | lo)};
    check_timestamp_range<validate>(tmp);

    return tmp;
  }
};

/// Specialize jb::itch5::encoder<> for a timestamp
template <bool validate>
struct encoder<validate, timestamp> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static void w(std::size_t size, void* buf, std::size_t offset, timestamp x) {
    check_offset<validate>("encoder<timestamp>", size, offset, 6);
    check_timestamp_range<validate>(x);
    using std::chrono::duration_cast;
    using std::chrono::nanoseconds;
    std::uint64_t nanos = duration_cast<nanoseconds>(x.ts).count();
    std::uint16_t hi = nanos >> 32;
    std::uint32_t lo = nanos & 0xFFFFFFFF;
    encoder<false, std::uint16_t>::w(size, buf, offset, hi);
    encoder<false, std::uint32_t>::w(size, buf, offset + 2, lo);
  }
};

/// Streaming operator for jb::itch5::timestamp.
std::ostream& operator<<(std::ostream& os, timestamp const& x);

} // namespace itch5
} // namespace jb

#endif // jb_itch5_timestamp_hpp
