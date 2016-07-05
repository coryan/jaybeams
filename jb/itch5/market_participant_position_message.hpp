#ifndef jb_itch5_market_participant_position_message_hpp
#define jb_itch5_market_participant_position_message_hpp

#include <jb/itch5/char_list_field.hpp>
#include <jb/itch5/message_header.hpp>
#include <jb/itch5/mpid_field.hpp>
#include <jb/itch5/stock_field.hpp>

namespace jb {
namespace itch5 {

/**
 * Represent the 'Primary Market Maker' field on a 'Market Participant
 * Position' message.
 */
typedef char_list_field<u'Y', u'N'> primary_market_maker_t;

/**
 * Represent the 'Market Maker Mode' field in a 'Market Participant
 * Position' message.
 */
typedef char_list_field<
  u'N', // Normal
  u'P', // Passive
  u'S', // Syndicate
  u'R', // Pre-syndicate
  u'L'  // Penalty
  > market_maker_mode_t;

/**
 * Represent the 'Market Participant State' field in a 'Market Participant
 * Position' message.
 */
typedef char_list_field<
  u'A', // Active
  u'E', // Excused/Withdrawn
  u'W', // Withdrawn
  u'S', // Suspended
  u'D'  // Deleted
  > market_participant_state_t;

/**
 * Represent a 'Market Participant Position' message in the ITCH-5.0 protocol.
 */
struct market_participant_position_message {
  constexpr static int message_type = u'L';

  message_header header;
  mpid_t mpid;
  stock_t stock;
  primary_market_maker_t primary_market_maker;
  market_maker_mode_t market_maker_mode;
  market_participant_state_t market_participant_state;
};

/// Specialize decoder for a jb::itch5::market_participant_position_message
template<bool V>
struct decoder<V,market_participant_position_message> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static market_participant_position_message r(
      std::size_t size, void const* buf, std::size_t off) {
    market_participant_position_message x;
    x.header =
        decoder<V,message_header>             ::r(size, buf, off + 0);
    x.mpid =
        decoder<V,mpid_t>                     ::r(size, buf, off + 11);
    x.stock =
        decoder<V,stock_t>                    ::r(size, buf, off + 15);
    x.primary_market_maker =
        decoder<V,primary_market_maker_t>     ::r(size, buf, off + 23);
    x.market_maker_mode =
        decoder<V,market_maker_mode_t>        ::r(size, buf, off + 24);
    x.market_participant_state =
        decoder<V,market_participant_state_t> ::r(size, buf, off + 25);
    return x;
  }
};

/// Streaming operator for jb::itch5::market_participant_position_message.
std::ostream& operator<<(
    std::ostream& os, market_participant_position_message const& x);

} // namespace itch5
} // namespace jb

#endif // jb_itch5_market_participant_position_message_hpp
