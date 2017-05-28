#ifndef jb_pitch2_base_add_order_message_hpp
#define jb_pitch2_base_add_order_message_hpp

#include <jb/fixed_string.hpp>
#include <boost/endian/buffers.hpp>

#include <iosfwd>
#include <utility>

namespace jb {
namespace pitch2 {

/**
 * Common type for the 'Add Order' messages in the PITCH-2.X protocol.
 *
 * The protocol defines 3 different 'Add Order' messages, which are
 * largely identical except for the width of some of the fields.  We
 * use this template class to represent the common structure of these
 * messages.
 */
template <typename quantity_type, typename price_type>
struct base_add_order_message {
  /// The type for the symbol field.
  using symbol_type = jb::fixed_string<6>;

  boost::endian::little_uint8_buf_t length;
  boost::endian::little_uint8_buf_t message_type;
  boost::endian::little_int32_buf_t time_offset;
  boost::endian::little_uint64_buf_t order_id;
  boost::endian::little_uint8_buf_t side_indicator;
  quantity_type quantity;
  symbol_type symbol;
  price_type price;
  boost::endian::little_uint8_buf_t add_flags;
};

} // namespace pitch2
} // namespace jb

#endif // jb_pitch2_base_add_order_message_hpp
