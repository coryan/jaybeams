#include "jb/itch5/array_based_order_book.hpp"

#include <sstream>

namespace jb {
namespace itch5 {

namespace defaults {
#ifndef JB_ITCH5_DEFAULTS_array_based_book_max_size
#define JB_ITCH5_DEFAULTS_array_based_book_max_size 8192
#endif // JB_ITCH5_DEFAULTS_array_based_book_max_size

int max_size = JB_ITCH5_DEFAULTS_array_based_book_max_size;
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
  if (max_size() <= 0) {
    std::ostringstream os;
    os << "max-size option must be > 0, value=" << max_size();
    throw jb::usage(os.str(), 1);
  }
}

namespace detail {
void raise_invalid_operation_parameters(
    char const *operation, int qty, price4_t px) {
  std::ostringstream os;
  os << "array_based_book_side::" << operation << " - parameters out of range:"
     << " px=" << px << " should be in [" << price4_t(0) << ","
     << max_price_field_value<price4_t>() << "), qty=" << qty
     << " should be >= 0";
  throw jb::feed_error(os.str());
}

void raise_invalid_reduce(
    std::string const& msg, std::size_t tk_begin_top, std::size_t tk_inside,
    price4_t px, int book_qty, int qty) {
  std::ostringstream os;
  os << msg << " tk_begin_top=" << tk_begin_top << ", tk_inside=" << tk_inside
     << ", px=" << px << ", book_qty=" << book_qty << ", qty=" << qty;
  throw jb::feed_error(os.str());
}
} // namespace detail
} // namespace itch5
} // namespace jb
