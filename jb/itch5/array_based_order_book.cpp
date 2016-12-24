#include <jb/itch5/array_based_order_book.hpp>

namespace jb {
namespace itch5 {

/// check that parameters are valid
void validate_add_order_params(int qty, price4_t px) {
  if (qty <= 0 or px >= max_price_field_value<price4_t>()) {
    std::ostringstream os;
    os << "array_based_order_book::validate_add_order_params out of range."
       << " px=" << px << " qty=" << qty;
    throw jb::feed_error(os.str());
  }
}

/// Validate the configuration
void array_based_order_book::config::validate() const {
  if ((max_size() <= 0) or (max_size() > defaults::Max_Size)) {
    std::ostringstream os;
    os << "max-size must be > 0 and <=" << defaults::Max_Size
       << ", value=" << max_size();
    throw jb::usage(os.str(), 1);
  }
}
} // namespace itch5
} // namespace jb
