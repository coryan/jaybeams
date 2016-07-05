#ifndef jb_itch5_reg_sho_restriction_message_hpp
#define jb_itch5_reg_sho_restriction_message_hpp

#include <jb/itch5/char_list_field.hpp>
#include <jb/itch5/message_header.hpp>
#include <jb/itch5/stock_field.hpp>

namespace jb {
namespace itch5 {

/**
 * Represent the 'Reg SHO Action' field of the 'Reg SHO Restriction'
 * message.
 */
typedef char_list_field<u'0', u'1', u'2'> reg_sho_action_t;

/**
 * Represent a 'Reg SHO Restriction' message in the ITCH-5.0 protocol.
 */
struct reg_sho_restriction_message {
  constexpr static int const message_type = u'Y';

  message_header header;
  stock_t stock;
  reg_sho_action_t reg_sho_action;
};

/// Specialize decoder for a jb::itch5::reg_sho_restriction_message
template<bool V>
struct decoder<V,reg_sho_restriction_message> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static reg_sho_restriction_message r(
      std::size_t size, void const* buf, std::size_t off) {
    reg_sho_restriction_message x;
    x.header =
        decoder<V,message_header>   ::r(size, buf, off + 0);
    x.stock =
        decoder<V,stock_t>          ::r(size, buf, off + 11);
    x.reg_sho_action =
        decoder<V,reg_sho_action_t> ::r(size, buf, off + 19);
    return x;
  }
};

/// Streaming operator for jb::itch5::reg_sho_restriction_message.
std::ostream& operator<<(
    std::ostream& os, reg_sho_restriction_message const& x);

} // namespace itch5
} // namespace jb

#endif // jb_itch5_reg_sho_restriction_message_hpp
