#ifndef jb_pitch2_unit_clear_message_hpp
#define jb_pitch2_unit_clear_message_hpp

#include <boost/endian/buffers.hpp>

#include <iosfwd>
#include <utility>

namespace jb {
namespace pitch2 {

/**
 * Represent the 'Unit Clear' message in the PITCH-2.X protocol.
 *
 * A full description of the fields can be found in the BATS PITCH-2.X
 * specification:
 *   https://www.batstrading.com/resources/membership/BATS_MC_PITCH_Specification.pdf
 */
struct unit_clear_message {
  /// Define the messsage type
  constexpr static int type = 0x97;

  boost::endian::little_uint8_buf_t length;
  boost::endian::little_uint8_buf_t message_type;
  boost::endian::little_uint32_buf_t time_offset;
};

/// Streaming operator for jb::pitch2::unit_clear_message.
std::ostream& operator<<(std::ostream& os, unit_clear_message const& x);

} // namespace pitch2
} // namespace jb

#endif // jb_pitch2_unit_clear_message_hpp
