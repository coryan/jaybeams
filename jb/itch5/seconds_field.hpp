#ifndef jb_itch5_seconds_field_hpp
#define jb_itch5_seconds_field_hpp

#include <jb/itch5/base_decoders.hpp>
#include <chrono>
#include <iosfwd>

namespace jb {
namespace itch5 {

/**
 * Represent a ITCH-5.0 seconds_field
 *
 * ITCH-5.0 uses seconds since midnight for some of its fields.
 */
class seconds_field {
 public:
  /// Constructor
  explicit seconds_field(int c = 0)
      : count_(c) {
  }

  /// Constructor from std::chrono::seconds
  explicit seconds_field(std::chrono::seconds const& s)
      : count_(s.count()) {
  }

  //@{
  /**
   * @name Accessors
   */
  int int_seconds() const {
    return count_;
  }

  std::chrono::seconds seconds() const {
    return std::chrono::seconds(count_);
  }
  //@}

 private:
  int count_;
};

/**
 * Validate the a seconds_field value.
 *
 * In ITCH-5.0 messages the seconds_field represents nanoseconds since
 * midnight.  The protocol is designed to start new sessions at the
 * beginning of each day, so seconds_fields cannot ever be more than 24
 * hours in nanoseconds.
 *
 * @tparam validate unless it is true the function is a no-op.
 *
 * @throws std::runtime_error if @a tparam is true and the seconds_field
 * is out of the expected range.
 */
template<bool validate>
void check_seconds_field_range(seconds_field const& t) {
}

/**
 * Provide an active implementation of
 * jb::itch5::check_seconds_field_range<>
 */
template<>
void check_seconds_field_range<true>(seconds_field const& t);

/// Specialize jb::itch5::decoder<> for a seconds_field
template<bool validate>
struct decoder<validate,seconds_field> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static seconds_field r(
      std::size_t size, void const* buf, std::size_t offset) {
    std::uint64_t ts = decoder<validate,std::uint32_t>::r(size, buf, offset);
    seconds_field tmp(ts);
    check_seconds_field_range<validate>(tmp);

    return tmp;
  }
};

/// Streaming operator for jb::itch5::seconds_field.
std::ostream& operator<<(std::ostream& os, seconds_field const& x);

} // namespace itch5
} // namespace jb

#endif // jb_itch5_seconds_field_hpp
