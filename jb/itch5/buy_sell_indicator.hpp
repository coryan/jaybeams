#ifndef jb_itch5_buy_sell_indicator_hpp
#define jb_itch5_buy_sell_indicator_hpp

#include <jb/itch5/char_list_field.hpp>

namespace jb {
namespace itch5 {

/**
 * Represent the 'Buy/Sell Indicator' field on several messages.
 */
typedef char_list_field<u'S', u'B'> buy_sell_indicator_t;

} // namespace itch5
} // namespace jb

#endif /* jb_itch5_buy_sell_indicator_hpp */

