#include "jb/itch5/array_based_order_book.hpp"
#include <jb/itch5/price_field.hpp>
#include <jb/itch5/quote_defaults.hpp>
#include <jb/feed_error.hpp>
#include <sstream>

namespace jb {
namespace itch5 {

namespace defaults {

#ifndef JB_ITCH5_DEFAULTS_array_based_book_max_size
#define JB_ITCH5_DEFAULTS_array_based_book_max_size 8192
#endif // JB_ITCH5_DEFAULTS_array_based_book_max_size

std::size_t max_size = JB_ITCH5_DEFAULTS_array_based_book_max_size;
} // namespace defaults

array_based_order_book::config::config()
    : max_size(
          desc("max-size")
              .help(
                  "Configure the max size of a array based order book."
                  " Only used when enable-array-based is set"),
          this, jb::itch5::defaults::max_size) {
}

void array_based_order_book::config::validate() const {
  if ((max_size() <= 0) or (max_size() > jb::itch5::defaults::max_size)) {
    std::ostringstream os;
    os << "max-size must be > 0 and <=" << jb::itch5::defaults::max_size
       << ", value=" << max_size();
    throw jb::usage(os.str(), 1);
  }
}

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
