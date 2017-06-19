#ifndef jb_pitch2_order_executed_message_hpp
#define jb_pitch2_order_executed_message_hpp

#include <boost/endian/buffers.hpp>

#include <iosfwd>

namespace jb {
namespace pitch2 {

/**
 * Represent the 'Order Executed' message in the PITCH-2.X protocol.
 *
 * A full description of the fields can be found in the BATS PITCH-2.X
 * specification:
 *   https://www.batstrading.com/resources/membership/BATS_MC_PITCH_Specification.pdf
 */
struct order_executed_message {
  /// Define the messsage type
  constexpr static int type = 0x23;

  boost::endian::little_uint8_buf_t length;
  boost::endian::little_uint8_buf_t message_type;
  boost::endian::little_uint32_buf_t time_offset;
  boost::endian::little_uint64_buf_t order_id;
  boost::endian::little_uint32_buf_t executed_quantity;
  boost::endian::little_uint64_buf_t execution_id;
};

/// Streaming operator for jb::pitch2::order_executed_message.
std::ostream& operator<<(std::ostream& os, order_executed_message const& x);

} // namespace pitch2
} // namespace jb

#endif // jb_pitch2_order_executed_message_hpp
