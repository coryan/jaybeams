#ifndef jb_itch5_static_digits_hpp
#define jb_itch5_static_digits_hpp

#include <cstdint>

namespace jb {
namespace itch5 {

/// Compute (at compile time if possible) the number of digits in a
/// number.
constexpr int static_digits(std::intmax_t value) {
  return value < 10 ? 1 : 1 + static_digits(value / 10);
}

} // namespace itch5
} // namespace jb

#endif /* jb_itch5_static_digits_hpp */
