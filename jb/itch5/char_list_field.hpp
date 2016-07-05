#ifndef jb_itch5_char_list_field_hpp
#define jb_itch5_char_list_field_hpp

#include <jb/itch5/char_list_validator.hpp>
#include <jb/itch5/base_decoders.hpp>

#include <boost/operators.hpp>

#include <iostream>
#include <cctype>

namespace jb {
namespace itch5 {

/**
 * A helper type to define char fields with a limited set of values.
 *
 * Many ITCH-5.0 fields are represented by a single byte on the wire,
 * and are only supposed to take a limited set of values.
 */
template<int...V>
class char_list_field
    : public boost::less_than_comparable<char_list_field<V...>>
    , public boost::less_than_comparable<char_list_field<V...>, int>
    , public boost::equality_comparable<char_list_field<V...>>
    , public boost::equality_comparable<char_list_field<V...>, int> {
 public:
  /// Default constructor
  char_list_field()
      : value_() {
  }

  /**
   * Constructor from an integer value.
   *
   * @throws std::runtime_exception if the value is not in the
   * template parameter list
   */
  explicit char_list_field(int x)
      : value_(x) {
    char_list_validator<true,V...> validator;
    validator(value_);
  }

  /// Return the integer value
  int as_int() const {
    return value_;
  }

  //@{
  /**
   * @name Comparison operators
   */
  bool operator==(char_list_field const& rhs) const {
    return value_ == rhs.value_;
  }
  bool operator==(int rhs) const {
    return value_ == rhs;
  }
  bool operator<(char_list_field const& rhs) const {
    return value_ < rhs.value_;
  }
  bool operator<(int rhs) const {
    return value_ < rhs;
  }
  //@}

 private:
  friend struct decoder<true,char_list_field>;
  friend struct decoder<false,char_list_field>;

  /// In-memory representation of the field (int).  Typically ints are
  /// more efficient (in CPU time) than 8-bits octet.
  int value_;
};

/// Specialize decoder<bool,T> for char_list_field.
template<bool validate, int...V>
struct decoder<validate,char_list_field<V...>> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static char_list_field<V...> r(
      std::size_t size, void const* buf, std::size_t offset) {
    char_list_field<V...> tmp;
    tmp.value_ = decoder<false,std::uint8_t>::r(size, buf, offset);

    char_list_validator<validate,V...> validator;
    validator(tmp.as_int());
    return tmp;
  }
};

/// Streaming operator for jb::itch5::char_list_field<>
template<int...V>
std::ostream& operator<<(std::ostream& os, char_list_field<V...> const& x) {
  if (std::isprint(x.as_int())) {
    return os << static_cast<char>(x.as_int());
  }
  return os << ".(" << x.as_int() << ")";
}

} // namespace itch5
} // namespace jb

#endif // jb_itch5_char_list_field_hpp
