#ifndef jb_itch5_add_order_mpid_message_hpp
#define jb_itch5_add_order_mpid_message_hpp

#include <jb/itch5/add_order_message.hpp>
#include <jb/itch5/mpid_field.hpp>

namespace jb {
namespace itch5 {

/**
 * Represent an 'Add Order with MPID' message in the ITCH-5.0 protocol.
 */
struct add_order_mpid_message : public add_order_message {
  constexpr static int message_type = u'F';

  mpid_t attribution;

  add_order_mpid_message(
      add_order_message const& base, mpid_t const& a)
      : add_order_message(base)
      , attribution(a)
  {}

  add_order_mpid_message() = default;
  add_order_mpid_message(add_order_mpid_message const&) = default;
  add_order_mpid_message(add_order_mpid_message&&) = default;
  add_order_mpid_message& operator=(add_order_mpid_message const&) = default;
  add_order_mpid_message& operator=(add_order_mpid_message&&) = default;

  add_order_mpid_message& operator=(add_order_message const& rhs) {
    static_cast<add_order_message*>(this)->operator=(rhs);
    return *this;
  }
  add_order_mpid_message& operator=(add_order_message&& rhs) {
    static_cast<add_order_message*>(this)->operator=(rhs);
    return *this;
  }
};

/// Specialize decoder for a jb::itch5::add_order_mpid_message
template<bool V>
struct decoder<V,add_order_mpid_message> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static add_order_mpid_message r(
      std::size_t size, char const* buf, std::size_t off) {
    add_order_mpid_message x;
    x =             decoder<V,add_order_message> ::r(size, buf, off + 0);
    x.attribution = decoder<V,mpid_t>            ::r(size, buf, off + 36);
    return std::move(x);
  }
};

/// Streaming operator for jb::itch5::add_order_mpid_message.
std::ostream& operator<<(
    std::ostream& os, add_order_mpid_message const& x);

} // namespace itch5
} // namespace jb

#endif /* jb_itch5_add_order_mpid_message_hpp */
