#ifndef jb_itch5_timestamp_hpp
#define jb_itch5_timestamp_hpp

#include <jb/itch5/base_decoders.hpp>
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
template<bool validate>
void check_timestamp_range(timestamp const& t) {
}

/**
 * Provide an active implementation of
 * jb::itch5::check_timestamp_range<>
 */
template<>
void check_timestamp_range<true>(timestamp const& t);

/// Specialize jb::itch5::decoder<> for a timestamp
template<bool validate>
struct decoder<validate,timestamp> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static timestamp r(std::size_t size, char const* buf, std::size_t offset) {
    check_offset<validate>("timestamp", size, offset, 6);
    std::uint64_t hi = decoder<false,std::uint32_t>::r(size, buf, offset);
    std::uint64_t lo = decoder<false,std::uint16_t>::r(size, buf, offset + 4);
    timestamp tmp{std::chrono::nanoseconds(hi << 16 | lo)};
    check_timestamp_range<validate>(tmp);

    return tmp;
  }
};

/// Streaming operator for jb::itch5::timestamp.
std::ostream& operator<<(std::ostream& os, timestamp const& x);

} // namespace itch5
} // namespace jb

#endif // jb_itch5_timestamp_hpp
