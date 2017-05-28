#ifndef jb_pitch2_short_add_order_message_hpp
#define jb_pitch2_short_add_order_message_hpp

#include <jb/pitch2/base_add_order_message.hpp>

namespace jb {
namespace pitch2 {

/**
 * Represent the short version of 'Add Order' message in the PITCH-2.X protocol.
 *
 * Sometimes the specification refers to this message as 'Add Order -
 * short'.
 *
 * A full description of the fields can be found in the BATS PITCH-2.X
 * specification:
 *   https://www.batstrading.com/resources/membership/BATS_MC_PITCH_Specification.pdf
 */
struct short_add_order_message
    : public base_add_order_message<boost::endian::little_uint16_buf_t,
                                    boost::endian::little_uint16_buf_t> {
  /// Define the messsage type
  constexpr static int type = 0x21;
};

/// Streaming operator for jb::pitch2::short_add_order_message.
std::ostream& operator<<(std::ostream& os, short_add_order_message const& x);

} // namespace pitch2
} // namespace jb

#endif // jb_pitch2_short_add_order_message_hpp
