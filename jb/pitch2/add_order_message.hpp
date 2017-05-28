#ifndef jb_pitch2_add_order_message_hpp
#define jb_pitch2_add_order_message_hpp

#include <jb/fixed_string.hpp>
#include <boost/endian/buffers.hpp>

#include <iosfwd>
#include <utility>

namespace jb {
namespace pitch2 {

/**
 * Represent the 'Add Order' message in the PITCH-2.X protocol.
 *
 * A full description of the fields can be found in the BATS PITCH-2.X
 * specification:
 *   https://www.batstrading.com/resources/membership/BATS_MC_PITCH_Specification.pdf
 */
struct add_order_message {
  /// Define the messsage type
  constexpr static int type = 0x21;

  /// The type for the symbol field.
  using symbol_type = jb::fixed_string<6>;

  boost::endian::little_uint8_buf_t length;
  boost::endian::little_uint8_buf_t message_type;
  boost::endian::little_int32_buf_t time_offset;
  boost::endian::little_uint64_buf_t order_id;
  boost::endian::little_uint8_buf_t side_indicator;
  boost::endian::little_uint32_buf_t quantity;
  symbol_type symbol;
  boost::endian::little_uint64_buf_t price;
  boost::endian::little_uint8_buf_t add_flags;
};

/// Streaming operator for jb::pitch2::add_order_message.
std::ostream& operator<<(std::ostream& os, add_order_message const& x);

} // namespace pitch2
} // namespace jb

#endif // jb_pitch2_aution_update_message_hpp
