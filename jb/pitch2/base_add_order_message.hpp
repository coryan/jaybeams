#ifndef jb_pitch2_base_add_order_message_hpp
#define jb_pitch2_base_add_order_message_hpp

#include <jb/fixed_string.hpp>
#include <boost/endian/buffers.hpp>

namespace jb {
namespace pitch2 {

/**
 * Common type for the 'Add Order' messages in the PITCH-2.X protocol.
 *
 * The protocol defines 3 different 'Add Order' messages, which are
 * largely identical except for the width of some of the fields.  We
 * use this template class to represent the common structure of these
 * messages.
 *
 * @tparam quantity_t the type used for the quantity field.
 * @tparam price_t the type used for the price field.
 * @tparam symbol_t the type used for the symbol field.
 */
template <typename quantity_t, typename symbol_t, typename price_t>
struct base_add_order_message {
  /// Capture the quantity_type template parameter as a trait
  using quantity_type = quantity_t;

  /// Capture the price_type template parameter as a trait
  using price_type = price_t;

  /// The type for the symbol field.
  using symbol_type = symbol_t;

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
