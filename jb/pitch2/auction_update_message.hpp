#ifndef jb_pitch2_auction_update_message_hpp
#define jb_pitch2_auction_update_message_hpp

#include <jb/fixed_string.hpp>
#include <boost/endian/buffers.hpp>

#include <iosfwd>
#include <utility>

namespace jb {
namespace pitch2 {

/**
 * Represent the 'Auction Update' message in the PITCH-2.X protocol.
 *
 * A full description of the fields can be found in the BATS PITCH-2.X
 * specification:
 *   https://www.batstrading.com/resources/membership/BATS_MC_PITCH_Specification.pdf
 */
struct auction_update_message {
  /// Define the messsage type
  constexpr static int type = 0x95;

  /// The type for the stock_symbol field.
  using stock_symbol_type = jb::fixed_string<8>;

  boost::endian::little_uint8_buf_t length;
  boost::endian::little_uint8_buf_t message_type;
  boost::endian::little_uint32_buf_t time_offset;
  stock_symbol_type stock_symbol;
  boost::endian::little_uint8_buf_t auction_type;
  boost::endian::little_uint64_buf_t reference_price;
  boost::endian::little_uint32_buf_t buy_shares;
  boost::endian::little_uint32_buf_t sell_shares;
  boost::endian::little_uint64_buf_t indicative_price;
  boost::endian::little_uint64_buf_t auction_only_price;
};

/// Streaming operator for jb::pitch2::auction_update_message.
std::ostream& operator<<(std::ostream& os, auction_update_message const& x);

} // namespace pitch2
} // namespace jb

#endif // jb_pitch2_auction_update_message_hpp
