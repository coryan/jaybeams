#ifndef jb_itch5_cross_type_hpp
#define jb_itch5_cross_type_hpp

#include <jb/itch5/char_list_field.hpp>

namespace jb {
namespace itch5 {

/**
 * Represent the 'Cross Type' field on 'Cross Trade' and 'Net Order
 * Imbalance Indicator' messages.
 */
typedef char_list_field<u'O', u'C', u'H', u'I'> cross_type_t;

} // namespace itch5
} // namespace jb

#endif // jb_itch5_cross_type_hpp
