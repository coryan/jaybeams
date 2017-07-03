#ifndef jb_pitch2_expanded_add_order_message_hpp
#define jb_pitch2_expanded_add_order_message_hpp

#include <jb/pitch2/base_add_order_message.hpp>

namespace jb {
namespace pitch2 {

/**
 * Represent the 'Add Order' message in the PITCH-2.X protocol.
 *
 * Sometimes the specification refers to this message as 'Add Order -
 * expanded'.
 *
 * A full description of the fields can be found in the BATS PITCH-2.X
 * specification:
 *   https://www.batstrading.com/resources/membership/BATS_MC_PITCH_Specification.pdf
 */
struct expanded_add_order_message
    : public base_add_order_message<
          boost::endian::little_uint32_buf_t, jb::fixed_string<8>,
          boost::endian::little_uint64_buf_t> {
  /// Define the messsage type
  constexpr static int type = 0x2F;

  /// The type for the participant_id field.
  using participant_type = jb::fixed_string<4>;

  participant_type participant_id;
};

/// Streaming operator for jb::pitch2::expanded_add_order_message.
std::ostream& operator<<(std::ostream& os, expanded_add_order_message const& x);

} // namespace pitch2
} // namespace jb

#endif // jb_pitch2_expanded_add_order_message_hpp
