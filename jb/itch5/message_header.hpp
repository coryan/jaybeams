#ifndef jb_itch5_message_header_hpp
#define jb_itch5_message_header_hpp

#include <jb/itch5/timestamp.hpp>

#include <iosfwd>
#include <utility>

namespace jb {
namespace itch5 {

/**
 * Define the header common to all ITCH 5.0 messages.
 */
struct message_header {
  /**
   * The type of message.  ITCH-5.0 messages are identified by their
   * first byte, with an ASCII value assigned to each message.
   *
   * offset=0
   * width=1
   */
  int message_type;

  /**
   * The stock locate number.
   *
   * Every stock receives a unique number in an ITCH-5.0 session.  For
   * messages that are not stock specific, this value is 0.
   *
   * offset=1
   * width=2
   */
  int stock_locate;

  /**
   * The "Tracking Number", a field designed for "internal NASDAQ purposes".
   *
   * The ITCH-5.0 specification does not document how this field is to
   * be interpreted.
   *
   * offset=3
   * width=2
   */
  int tracking_number;

  /**
   * The message timestamp, in nanoseconds since midnight.
   *
   * All messages in a ITCH-5.0 session are timestamped, in
   * nanoseconds since midnight for whatever day the session started
   * running.  All sessions are terminated before the end of the day.
   *
   * offset=5
   * width=6
   */
  jb::itch5::timestamp timestamp;
};

/// Specialize decoder for a jb::itch5::message_header
template <bool validate> struct decoder<validate, message_header> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static message_header r(std::size_t size, void const* buf, std::size_t off) {
    message_header x;
    x.message_type = decoder<validate, std::uint8_t>::r(size, buf, off + 0);
    x.stock_locate = decoder<validate, std::uint16_t>::r(size, buf, off + 1);
    x.tracking_number = decoder<validate, std::uint16_t>::r(size, buf, off + 3);
    x.timestamp = decoder<validate, timestamp>::r(size, buf, off + 5);
    return x;
  }
};

/// Streaming operator for jb::itch5::message_header.
std::ostream& operator<<(std::ostream& os, message_header const& x);

} // namespace itch5
} // namespace jb

#endif // jb_itch5_message_header_hpp
