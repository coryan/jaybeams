#ifndef jb_itch5_broken_trade_message_hpp
#define jb_itch5_broken_trade_message_hpp

#include <jb/itch5/message_header.hpp>

namespace jb {
namespace itch5 {

/**
 * Represent an 'Broken Trade / Order Execution' message in the
 * ITCH-5.0 protocol.
 */
struct broken_trade_message {
  constexpr static int message_type = u'B';

  message_header header;
  std::uint64_t match_number;
};

/// Specialize decoder for a jb::itch5::broken_trade_message
template <bool V> struct decoder<V, broken_trade_message> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static broken_trade_message r(std::size_t size, void const* buf,
                                std::size_t off) {
    broken_trade_message x;
    x.header = decoder<V, message_header>::r(size, buf, off + 0);
    x.match_number = decoder<V, std::uint64_t>::r(size, buf, off + 11);
    return x;
  }
};

/// Streaming operator for jb::itch5::broken_trade_message.
std::ostream& operator<<(std::ostream& os, broken_trade_message const& x);

} // namespace itch5
} // namespace jb

#endif // jb_itch5_broken_trade_message_hpp
