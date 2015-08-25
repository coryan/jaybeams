#ifndef jb_itch5_mwcb_decline_level_message_hpp
#define jb_itch5_mwcb_decline_level_message_hpp

#include <jb/itch5/message_header.hpp>
#include <jb/itch5/price_field.hpp>

namespace jb {
namespace itch5 {

/**
 * Represent a 'MWCB Decline Level' message in the ITCH-5.0 protocol.
 *
 * The Market Wide Circuit Breakers (MWCB) are a mechanism to halt
 * trading if the market declines to such a level that some error is
 * reasonably suspected.  The mechanism defines three different
 * levels, with different consequences at each level.
 */
struct mwcb_decline_level_message {
  constexpr static int message_type = u'V';

  message_header header;
  price8_t level_1;
  price8_t level_2;
  price8_t level_3;
};

/// Specialize decoder for a jb::itch5::mwcb_decline_level_message
template<bool V>
struct decoder<V,mwcb_decline_level_message> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static mwcb_decline_level_message r(
      std::size_t size, char const* buf, std::size_t off) {
    mwcb_decline_level_message x;
    x.header =
        decoder<V,message_header>  ::r(size, buf, off + 0);
    x.level_1 =
        decoder<V,price8_t>       ::r(size, buf, off + 11);
    x.level_2 =
        decoder<V,price8_t>       ::r(size, buf, off + 19);
    x.level_3 =
        decoder<V,price8_t>       ::r(size, buf, off + 27);
    return std::move(x);
  }
};

/// Streaming operator for jb::itch5::mwcb_decline_level_message.
std::ostream& operator<<(
    std::ostream& os, mwcb_decline_level_message const& x);

} // namespace itch5
} // namespace jb

#endif /* jb_itch5_mwcb_decline_level_message_hpp */
