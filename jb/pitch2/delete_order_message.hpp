#ifndef jb_pitch2_delete_message_hpp
#define jb_pitch2_delete_message_hpp

#include <boost/endian/buffers.hpp>

#include <iosfwd>

namespace jb {
namespace pitch2 {

/**
 * Represent the 'Delete Order' message in the PITCH-2.X protocol.
 *
 * A full description of the fields can be found in the BATS PITCH-2.X
 * specification:
 *   https://www.batstrading.com/resources/membership/BATS_MC_PITCH_Specification.pdf
 */
struct delete_order_message {
  /// Define the messsage type
  constexpr static int type = 0x29;

  boost::endian::little_uint8_buf_t length;
  boost::endian::little_uint8_buf_t message_type;
  boost::endian::little_uint32_buf_t time_offset;
  boost::endian::little_uint64_buf_t order_id;
};

/// Streaming operator for jb::pitch2::delete_message.
std::ostream& operator<<(std::ostream& os, delete_order_message const& x);

} // namespace pitch2
} // namespace jb

#endif // jb_pitch2_delete_message_hpp
