#include <jb/itch5/price_field.hpp>
#include <jb/itch5/quote_defaults.hpp>
#include <jb/feed_error.hpp>
#include <sstream>

namespace jb {
namespace itch5 {

/// check that parameters are valid
void validate_add_order_params(int qty, price4_t px) {
  if (qty <= 0 or px >= max_price_field_value<price4_t>()) {
    std::ostringstream os;
    os << "array_based_order_book::add_order value(s) out of range."
       << " px=" << px << " qty=" << qty;
    throw jb::feed_error(os.str());
  }
}

} // namespace itch5
} // namespace jb
