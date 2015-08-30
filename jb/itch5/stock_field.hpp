#ifndef jb_itch5_stock_field_hpp
#define jb_itch5_stock_field_hpp

#include <jb/itch5/short_string_field.hpp>

namespace jb {
namespace itch5 {

/**
 * Define the type used to represent the 'Stock' field in many
 * messages.
 *
 * All the ITCH-5.0 messages that carry stock-specific information use
 * an 8-byte alpha field to represent the stock symbol.  Please see
 * @ref securitiesvssymbols for some commentary on NASDAQ's naming 
 * conventions.
 */
typedef jb::itch5::short_string_field<8> stock_t;

} // namespace itch5
} // namespace jb

#endif /* jb_itch5_stock_field_hpp */

