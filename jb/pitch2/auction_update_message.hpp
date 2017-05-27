#ifndef jb_pitch2_message_header_hpp
#define jb_pitch2_message_header_hpp

#include <boost/endian/buffers.hpp>

#include <iosfwd>
#include <utility>

namespace jb {
namespace pitch2 {

/**
 * Represent the 'Auction Update' message in the PITCH-2.X protocol.
 */
struct auction_update_message {
  boost::endian::little_uint8_buf_t length;
  boost::endian::little_uint8_buf_t message_type;
  boost::endian::little_int32_buf_t time_offset;
  char stock_symbol[8];
  boost::endian::little_uint8_buf_t auction_type;
  boost::endian::little_uint64_buf_t reference_price;
  boost::endian::little_uint32_buf_t buy_shares;
  boost::endian::little_uint32_buf_t sell_shares;
  boost::endian::little_uint64_buf_t indicative_price;
  boost::endian::little_uint64_buf_t auction_only_price;
};

/// Streaming operator for jb::pitch::aution_update_message.
std::ostream& operator<<(std::ostream& os, auction_update_message const& x);

} // namespace pitch2
} // namespace jb

#endif // jb_pitch2_aution_update_message_hpp
