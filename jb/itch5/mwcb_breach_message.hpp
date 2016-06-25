#ifndef jb_itch5_mwcb_breach_message_hpp
#define jb_itch5_mwcb_breach_message_hpp

#include <jb/itch5/char_list_field.hpp>
#include <jb/itch5/message_header.hpp>

namespace jb {
namespace itch5 {

/**
 * Represent the 'Breached Level' field in the 'MWCB Breach Message'.
 */
typedef char_list_field<u'1', u'2', u'3'> breached_level_t;

/**
 * Represent a 'MWCB Breach' message in the ITCH-5.0 protocol.
 *
 * The Market Wide Circuit Breakers (MWCB) are a mechanism to halt
 * trading if the market declines to such a level that some error is
 * reasonably suspected.  The mechanism defines three different
 * levels, with different consequences at each level.
 */
struct mwcb_breach_message {
  constexpr static int message_type = u'W';

  message_header header;
  breached_level_t breached_level;
};

/// Specialize decoder for a jb::itch5::mwcb_breach_message
template<bool V>
struct decoder<V,mwcb_breach_message> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static mwcb_breach_message r(
      std::size_t size, char const* buf, std::size_t off) {
    mwcb_breach_message x;
    x.header =
        decoder<V,message_header>   ::r(size, buf, off + 0);
    x.breached_level =
        decoder<V,breached_level_t> ::r(size, buf, off + 11);
    return x;
  }
};

/// Streaming operator for jb::itch5::mwcb_breach_message.
std::ostream& operator<<(
    std::ostream& os, mwcb_breach_message const& x);

} // namespace itch5
} // namespace jb

#endif /* jb_itch5_mwcb_breach_message_hpp */
