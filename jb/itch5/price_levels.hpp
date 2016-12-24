#ifndef jb_itch5_price_levels_hpp
#define jb_itch5_price_levels_hpp

#include <jb/itch5/price_field.hpp>

#include <stdexcept>

namespace jb {
namespace itch5 {

/**
 * Compute the number of price levels between two prices.
 *
 * @param lo the low end of the range of price levels
 * @param hi the hi end of the range of price levels
 * @returns the number of price levels between @a lo and @a hi
 * @throws std::bad_range if @a hi < @a lo
 */
template <typename price_field_t>
std::size_t price_levels(price_field_t lo, price_field_t hi) {
  static_assert(
      price_field_t::denom >= 10000,
      "price_levels() does not work with denom < 10000");
  static_assert(
      price_field_t::denom % 10000 == 0,
      "price_levels() does not work with (denom % 10000) != 0");

  price_field_t const unit = price_field_t::dollar_price();
  auto constexpr penny = price_field_t::denom / 100;
  auto constexpr mill = penny / 100;

  if (hi < lo) {
    throw std::range_error("invalid price range in price_levels()");
  }
  if (unit <= lo) {
    // ... range is above $1.0, return the number of levels for that
    // case ...
    return (hi.as_integer() - lo.as_integer()) / penny;
  }
  if (hi <= unit) {
    // ... range is below $1.0, return the number of levels for that
    // case ...
    return (hi.as_integer() - lo.as_integer()) / mill;
  }
  // ... split the analysis ...
  return price_levels(lo, unit) + price_levels(unit, hi);
}

/**
 * Compute the absolute price of a price level.
 *
 * @param p_level price level
 * @returns absolute price
 * @throws std::bad_range if p_level out of range
 */
template <typename price_field_t>
auto level_to_price(typename price_field_t::wire_type const p_level) {
  static_assert(
      price_field_t::denom >= 10000,
      "price_levels() does not work with denom < 10000");
  static_assert(
      price_field_t::denom % 10000 == 0,
      "price_levels() does not work with (denom % 10000) != 0");

  price_field_t const max_price = max_price_field_value<price_field_t>();
  std::size_t const max_level = price_levels(price_field_t(0), max_price);
  if (p_level > max_level) {
    throw std::range_error("invalid price range in price_levels()");
  }
  if (p_level <= price_field_t::denom) {
    return price_field_t(p_level);
  }
  int rem = (p_level - price_field_t::denom) * 100 + price_field_t::denom;
  return price_field_t(rem);
}

} // namespace itch5
} // namespace jb

#endif // jb_itch5_price_levels_hpp
