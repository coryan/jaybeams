#ifndef jb_pitch2_modify_message_hpp
#define jb_pitch2_modify_message_hpp

#include <boost/endian/buffers.hpp>

#include <iosfwd>

namespace jb {
namespace pitch2 {

/**
 * Represent the 'Modify' messages in the PITCH-2.X protocol.
 *
 * A full description of the fields can be found in the BATS PITCH-2.X
 * specification:
 *   https://www.batstrading.com/resources/membership/BATS_MC_PITCH_Specification.pdf
 *
 * @tparam quantity_t the type used for the quantity field.
 * @tparam price_t the type used for the price field.
 */
template <typename quantity_t, typename price_t>
struct modify_message {
  /// Capture the quantity_t template parameter as a trait
  using quantity_type = quantity_t;

  /// Capture the price_t template parameter as a trait
  using price_type = price_t;

  boost::endian::little_uint8_buf_t length;
  boost::endian::little_uint8_buf_t message_type;
  boost::endian::little_uint32_buf_t time_offset;
  boost::endian::little_uint64_buf_t order_id;
  quantity_type quantity;
  price_type price;
  boost::endian::little_uint8_buf_t modify_flags;
};

/**
 * Represent the 'Modify (long)' messages in the PITCH-2.X protocol.
 *
 * A full description of the fields can be found in the BATS PITCH-2.X
 * specification:
 *   https://www.batstrading.com/resources/membership/BATS_MC_PITCH_Specification.pdf
 */
struct modify_long_message : public modify_message<
                                 boost::endian::little_uint32_buf_t,
                                 boost::endian::little_uint64_buf_t> {
  /// Define the messsage type
  constexpr static int type = 0x27;
};

/// Streaming operator for jb::pitch2::modify_message.
std::ostream& operator<<(std::ostream& os, modify_long_message const& x);

/**
 * Represent the 'Modify (short)' messages in the PITCH-2.X protocol.
 *
 * A full description of the fields can be found in the BATS PITCH-2.X
 * specification:
 *   https://www.batstrading.com/resources/membership/BATS_MC_PITCH_Specification.pdf
 */
struct modify_short_message : public modify_message<
                                  boost::endian::little_uint16_buf_t,
                                  boost::endian::little_uint16_buf_t> {
  /// Define the messsage type
  constexpr static int type = 0x28;
};

/// Streaming operator for jb::pitch2::modify_message.
std::ostream& operator<<(std::ostream& os, modify_short_message const& x);

} // namespace pitch2
} // namespace jb

#endif // jb_pitch2_modify_message_hpp
