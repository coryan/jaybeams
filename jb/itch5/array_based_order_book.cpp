#include "jb/itch5/array_based_order_book.hpp"

#include <sstream>

namespace jb {
namespace itch5 {

namespace defaults {

#ifndef JB_ARRAY_DEFAULTS_max_size
#define JB_ARRAY_DEFAULTS_max_size 10000
#endif
std::size_t const max_size = JB_ARRAY_DEFAULTS_max_size;

} // namespace defaults

array_based_order_book::config::config()
    : max_size(
          desc("max-size")
              .help(
                  "Configure the max size of a array based order book."
                  " Only used when enable-array-based is set"),
          this, defaults::max_size) {
}

/// Validate the configuration
void array_based_order_book::config::validate() const {
  if ((max_size() <= 0) or (max_size() > defaults::max_size)) {
    std::ostringstream os;
    os << "max-size must be > 0 and <=" << defaults::max_size
       << ", value=" << max_size();
    throw jb::usage(os.str(), 1);
  }
}

/// check that parameters are valid
void validate_add_order_params(int qty, price4_t px) {
  if (qty <= 0 or px >= max_price_field_value<price4_t>()) {
    std::ostringstream os;
    os << "array_based_book_side::validate_add_order_params out of range."
       << " px=" << px << " qty=" << qty;
    throw jb::feed_error(os.str());
  }
}

/// handle exception with tk_begin_top_, px, and qty
void raise_exception(
    std::string str, std::size_t tk_begin_top, price4_t px, int qty) {
  std::ostringstream os;
  os << str << " tk_begin_top_=" << tk_begin_top << " px=" << px
     << " qty=" << qty;
  throw jb::feed_error(os.str());
}

/// handle exception with relative inside and px relative position
void raise_exception(
    std::string str, std::size_t tk_begin_top, std::size_t tk_inside,
    std::size_t rel_px, price4_t px, int qty) {
  std::ostringstream os;
  os << str << " tk_begin_top_=" << tk_begin_top << " tk_inside_" << tk_inside
     << " px relative position " << rel_px << " px=" << px << " qty=" << qty;
  throw jb::feed_error(os.str());
}
} // namespace itch5
} // namespace jb
