#ifndef jb_pitch2_reduce_size_message_hpp
#define jb_pitch2_reduce_size_message_hpp

#include <boost/endian/buffers.hpp>

#include <iosfwd>

namespace jb {
namespace pitch2 {

/**
 * Represent the 'Reduce Size' messages in the PITCH-2.X protocol.
 *
 * A full description of the fields can be found in the BATS PITCH-2.X
 * specification:
 *   https://www.batstrading.com/resources/membership/BATS_MC_PITCH_Specification.pdf
 *
 * @tparam quantity_t the type used for the canceled_quantity field.
 */
template <typename quantity_t>
struct reduce_size_message {
  /// Capture the quantity_t template parameter as a trait
  using quantity_type = quantity_t;

  boost::endian::little_uint8_buf_t length;
  boost::endian::little_uint8_buf_t message_type;
  boost::endian::little_uint32_buf_t time_offset;
  boost::endian::little_uint64_buf_t order_id;
  quantity_type canceled_quantity;
};

/**
 * Represent the 'Reduce Size (long)' messages in the PITCH-2.X protocol.
 *
 * A full description of the fields can be found in the BATS PITCH-2.X
 * specification:
 *   https://www.batstrading.com/resources/membership/BATS_MC_PITCH_Specification.pdf
 */
struct reduce_size_long_message
    : public reduce_size_message<boost::endian::little_uint32_buf_t> {
  /// Define the messsage type
  constexpr static int type = 0x25;
};

/// Streaming operator for jb::pitch2::reduce_size_message.
std::ostream& operator<<(std::ostream& os, reduce_size_long_message const& x);

/**
 * Represent the 'Reduce Size (short)' messages in the PITCH-2.X protocol.
 *
 * A full description of the fields can be found in the BATS PITCH-2.X
 * specification:
 *   https://www.batstrading.com/resources/membership/BATS_MC_PITCH_Specification.pdf
 */
struct reduce_size_short_message
    : public reduce_size_message<boost::endian::little_uint16_buf_t> {
  /// Define the messsage type
  constexpr static int type = 0x26;
};

/// Streaming operator for jb::pitch2::reduce_size_message.
std::ostream& operator<<(std::ostream& os, reduce_size_short_message const& x);

} // namespace pitch2
} // namespace jb

#endif // jb_pitch2_reduce_size_message_hpp
