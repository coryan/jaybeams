#ifndef jb_itch5_price_field_hpp
#define jb_itch5_price_field_hpp

#include <jb/itch5/base_decoders.hpp>
#include <jb/itch5/static_digits.hpp>

#include <boost/operators.hpp>

#include <iomanip>
#include <iostream>
#include <limits>
#include <ratio>

namespace jb {
namespace itch5 {

/**
 * Define a class used to represent prices in the ITCH-5.0 feed.
 *
 * The ITCH-5.0 protocol represent prices as fixed-point values.  The
 * number of decimal digits is specified in the field definition.  For
 * example, a Price(4) field represents prices with 4 decimal digits,
 * i.e., of the 99999.9999 form.  On the wire the number is actually
 * stored as an integer.  For example, the price 150.0100 would be
 * stored on the wire as 1500100.
 *
 * @tparam wire_type an integer type, typically std::uint32_t for
 *   4-byte wide fields, or std::uint64_t for 8-byte wide fields.
 * @tparam denom the denominator for this field, for example Price(4)
 *   would use denom==10000, while Price(8) would use 100000000.
 */
template <typename wire_type_t, std::intmax_t denom_v>
class price_field
    : public boost::equality_comparable<price_field<wire_type_t, denom_v>>,
      public boost::less_than_comparable<price_field<wire_type_t, denom_v>> {
public:
  /// The wire type
  typedef wire_type_t wire_type;

  /// The denominator
  constexpr static std::intmax_t denom = denom_v;

  /// The number of digits in the denominator
  constexpr static int denom_digits = static_digits(denom);

  /// The minimum quotation tick for this price
  typedef std::ratio<1, denom> tick;

  /// Default constructor
  price_field()
      : value_() {
  }

  /// Constructor from a price
  explicit price_field(wire_type rhs)
      : value_(rhs) {
  }

  /// Assignment from
  price_field& operator=(wire_type rhs) {
    value_ = rhs;
    return *this;
  }

  /// Addition assignment operator
  price_field& operator+=(price_field const& rhs) {
    value_ += rhs.value_;
    return *this;
  }

  //@{
  /**
   * @name Accessors
   */
  double as_double() const {
    return value_ / double(denom);
  }

  wire_type as_integer() const {
    return value_;
  }
  //@}

  //@{
  /**
   * Base comparison operators
   */
  bool operator==(price_field const& rhs) const {
    return value_ == rhs.value_;
  }

  bool operator<(price_field const& rhs) const {
    return value_ < rhs.value_;
  }
  //@}

  // Simple representation of $1.00
  static price_field dollar_price() {
    return price_field(price_field::denom);
  }

private:
  /// The value as an integer
  wire_type value_;
};

/// Specialize jb::itch5::decoder for jb::itch5::price_field
template <bool validate, typename wire_type_t, std::intmax_t denom_v>
struct decoder<validate, price_field<wire_type_t, denom_v>> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static price_field<wire_type_t, denom_v>
  r(std::size_t size, void const* buf, std::size_t offset) {
    price_field<wire_type_t, denom_v> tmp(
        decoder<validate, wire_type_t>::r(size, buf, offset));
    return tmp;
  }
};

/// Streaming operator for jb::itch5::price_field<>
template <typename wire_type_t, std::intmax_t denom_v>
std::ostream&
operator<<(std::ostream& os, price_field<wire_type_t, denom_v> const& x) {
  auto d = std::div(x.as_integer(), x.denom);
  return os << d.quot << "." << std::setw(x.denom_digits - 1)
            << std::setfill('0') << d.rem;
}

/// Convenience definition for Price(4) fields.
typedef price_field<std::uint32_t, 10000> price4_t;

/// Convenience definition for Price(8) fields.
typedef price_field<std::uint64_t, 100000000> price8_t;

template <typename price_field>
inline price_field max_price_field_value() {
  auto v = std::numeric_limits<typename price_field::wire_type>::max();
  v /= price_field::denom;
  return price_field(v * price_field::denom);
}

template <>
inline price4_t max_price_field_value() {
  // Per the ITCH-5.0 spec, the maximum value is 200,000.0000
  return price4_t(std::uint32_t(200000) * std::uint32_t(price4_t::denom));
}

/// non-member addition operator
template <typename price_field>
inline price_field operator+(price_field const& lhs, price_field const& rhs) {
  price_field temp = lhs;
  temp += rhs;
  return temp;
}

} // namespace itch5
} // namespace jb

#endif // jb_itch5_price_field_hpp
