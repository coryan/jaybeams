#ifndef jb_itch5_half_quote_hpp
#define jb_itch5_half_quote_hpp

#include <jb/itch5/price_field.hpp>

namespace jb {
namespace itch5 {

/// A simple representation for price + quantity
using half_quote = std::pair<price4_t, int>;

} // namespace itch5
} // namespace jb

#endif // jb_itch5_half_quote_hpp
