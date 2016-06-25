#ifndef jb_itch5_order_delete_message_hpp
#define jb_itch5_order_delete_message_hpp

#include <jb/itch5/message_header.hpp>

namespace jb {
namespace itch5 {

/**
 * Represent an 'Order Delete' message in the ITCH-5.0 protocol.
 */
struct order_delete_message {
  constexpr static int message_type = u'D';

  message_header header;
  std::uint64_t order_reference_number;
};

/// Specialize decoder for a jb::itch5::order_delete_message
template<bool V>
struct decoder<V,order_delete_message> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static order_delete_message r(
      std::size_t size, char const* buf, std::size_t off) {
    order_delete_message x;
    x.header =
        decoder<V,message_header>       ::r(size, buf, off + 0);
    x.order_reference_number =
        decoder<V,std::uint64_t>        ::r(size, buf, off + 11);
    return x;
  }
};

/// Streaming operator for jb::itch5::order_delete_message.
std::ostream& operator<<(
    std::ostream& os, order_delete_message const& x);

} // namespace itch5
} // namespace jb

#endif /* jb_itch5_order_delete_message_hpp */
