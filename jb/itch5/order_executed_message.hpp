#ifndef jb_itch5_order_executed_message_hpp
#define jb_itch5_order_executed_message_hpp

#include <jb/itch5/message_header.hpp>

namespace jb {
namespace itch5 {

/**
 * Represent an 'Order Executed' message in the ITCH-5.0 protocol.
 */
struct order_executed_message {
  constexpr static int message_type = u'E';

  message_header header;
  std::uint64_t order_reference_number;
  std::uint32_t executed_shares;
  std::uint64_t match_number;
};

/// Specialize decoder for a jb::itch5::order_executed_message
template<bool V>
struct decoder<V,order_executed_message> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static order_executed_message r(
      std::size_t size, void const* buf, std::size_t off) {
    order_executed_message x;
    x.header =
        decoder<V,message_header>       ::r(size, buf, off + 0);
    x.order_reference_number =
        decoder<V,std::uint64_t>        ::r(size, buf, off + 11);
    x.executed_shares =
        decoder<V,std::uint32_t>        ::r(size, buf, off + 19);
    x.match_number =
        decoder<V,std::uint64_t>        ::r(size, buf, off + 23);
    return x;
  }
};

/// Streaming operator for jb::itch5::order_executed_message.
std::ostream& operator<<(
    std::ostream& os, order_executed_message const& x);

} // namespace itch5
} // namespace jb

#endif // jb_itch5_order_executed_message_hpp
