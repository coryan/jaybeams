#ifndef jb_itch5_short_string_field_hpp
#define jb_itch5_short_string_field_hpp

#include <jb/itch5/decoder.hpp>
#include <jb/itch5/p2ceil.hpp>
#include <jb/itch5/noop_validator.hpp>

#include <boost/operators.hpp>
#include <boost/functional/hash.hpp>

#include <cstring>
#include <iostream>

namespace jb {
namespace itch5 {

/**
 * A helper type to define short (and fixed sized) string fields.
 *
 * The ITCH-5.0 protocol uses many fields that are short strings, that
 * is, fixed-length alpha numeric fields.  While it is normally easy
 * to treat these fields as character strings, the fields may not be
 * NUL terminated.  In fact, the protocol spec specifically states
 * that the fields will be left justified, and padded with spaces.
 *
 * We want an in-memory representation that is NUL terminated (for
 * compatibility with all the C/C++ libraries), and we do not wish to
 * allocate memory for these fields (as we would do if we used
 * std::string).
 *
 * @tparam wire_size_value the size of the field on the wire, in
 *   bytes.
 * @tparam value_validator a functor to validate the value for those
 *   fields that are only supposed to assume a known set of values.
 */
template<
  std::size_t wire_size_value,
  typename value_validator = noop_validator<char const*>>
class short_string_field
    : public boost::less_than_comparable<short_string_field<
                                           wire_size_value,value_validator>>
    , public boost::less_than_comparable<short_string_field<
                                           wire_size_value,value_validator>,
                                         char const*>
    , public boost::equality_comparable<short_string_field<
                                          wire_size_value,value_validator>>
    , public boost::equality_comparable<short_string_field<
                                          wire_size_value,value_validator>,
                                        char const*> {
 public:
  /// The size of the field on the wire
  constexpr static std::size_t wire_size = wire_size_value;

  /// The size of the field in memory
  constexpr static std::size_t buffer_size = jb::itch5::p2ceil(wire_size_value);

  /// The type of validator
  typedef value_validator value_validator_t;
  
  /// Constructor
  explicit short_string_field(
      value_validator_t const& validator = value_validator_t())
      : buffer_()
      , value_validator_(validator) {
  }

  /// Constructor from a std::string
  explicit short_string_field(
      std::string const& rhs,
      value_validator_t const& validator = value_validator_t())
      : buffer_()
      , value_validator_(validator) {
    std::strncpy(buffer_, rhs.c_str(), wire_size);
    nul_terminate();
  }

  /// Return the C-string representation
  char const* c_str() const {
    return buffer_;
  }


  //@{
  /**
   * @name Base comparison operators
   */
  /// compare vs another short_string_field
  bool operator==(short_string_field const& rhs) const {
    return std::strncmp(buffer_, rhs.buffer_, buffer_size) == 0;
  }

  /// compare vs another short_string_field
  bool operator<(short_string_field const& rhs) const {
    return std::strncmp(buffer_, rhs.buffer_, buffer_size) < 0;
  }

  /// compare vs a C string
  bool operator==(char const* rhs) const {
    return std::strncmp(buffer_, rhs, buffer_size) == 0;
  }

  /// compare vs a C string
  bool operator<(char const* rhs) const {
    return std::strncmp(buffer_, rhs, buffer_size) == 0;
  }
  //@}

  //@{
  /// @name default assignment and copy operators
  short_string_field(short_string_field const& rhs) = default;
  short_string_field(short_string_field&& rhs) = default;
  short_string_field& operator=(short_string_field const& rhs) = default;
  short_string_field& operator=(short_string_field&& rhs) = default;
  //@}

 private:
  friend struct jb::itch5::decoder<true,short_string_field>;
  friend struct jb::itch5::decoder<false,short_string_field>;
  /// Assignment from a character buffer
  void assign(char const* buf) {
    std::memcpy(buffer_, buf, wire_size);
    nul_terminate();
  }

  /// NUL terminate the string
  void nul_terminate() {
    // ... write a NUL character at one past the expected length on
    // the wire, we know this is safe because the buffer is always
    // larger than the wire size (by construction) ...
    buffer_[wire_size] = '\0';
    // ... find the first space character, on the wire the strings are
    // padded with spaces, we want to NUL terminate on the first
    // space.  We know this will not got beyond the buffer's end
    // because of the previous NUL terminator ...
    char* p = std::strchr(buffer_, u' ');
    // ... if there was a space, put a NUL character there ...
    if (p != nullptr) {
      *p = '\0';
    }
  }

  /**
   * Validate the value using the functor.
   *
   * @tparam enabled if false, disable all validation
   */
  void validate() const {
    if (not value_validator_(buffer_)) {
      jb::itch5::raise_validation_failed("short_string_field<>", buffer_);
    }
  }

 private:
  /// The in-memory representation
  char buffer_[buffer_size];

  /// The validator
  value_validator_t value_validator_;
};

/// Specialize decoder<bool,T> for short_string_field.
template<bool validate, std::size_t wsize, typename F>
struct decoder<validate, short_string_field<wsize,F>> {
  /// Please see the generic documentation for jb::itch5::decoder<>::r()
  static short_string_field<wsize,F> r(
      std::size_t size, char const* buffer, std::size_t offset) {
    jb::itch5::check_offset<validate>(
        "short_string_field<>", size, offset, wsize);

    short_string_field<wsize,F> tmp;
    tmp.assign(buffer + offset);
    if (validate) { tmp.validate(); }
    return tmp;
  }
};


/// Streaming operator for jb::itch5::short_string_field<>
template<std::size_t size, typename F>
std::ostream& operator<<(
    std::ostream& os, short_string_field<size,F> const& x) {
  return os << x.c_str();
}

/// Implement a hash function and integrate with boost::hash
template<std::size_t size, typename F>
std::size_t hash_value(short_string_field<size,F> const& x) {
  return boost::hash_range(x.c_str(), x.c_str() + size);
}

} // namespace itch5
} // namespace jb

#endif /* jb_itch5_short_string_field_hpp */
