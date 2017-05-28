#ifndef jb_fixed_string_hpp
#define jb_fixed_string_hpp

#include <boost/functional/hash.hpp>
#include <boost/operators.hpp>

#include <algorithm>
#include <cstring>
#include <ostream>
#include <type_traits>

namespace jb {

/**
 * A helper type to define short (and fixed sized) string fields.
 *
 * Many market data protocols uses fields that are short strings, that
 * is, fixed-length alpha numeric fields.  While it is normally easy
 * to treat these fields as character strings, the fields may not be
 * NUL terminated.  In fact, several protocols define such fields as
 * left justified, padded with spaces.
 *
 * We want an in-memory representation that supports basic comparison
 * operations, streaming to std::ostream, and conversion to
 * std::string.  We also want the C++ type to be a POD-type, so we can
 * copy the message buffer directly into memory.
 *
 * @tparam wire_size_value the size of the field on the wire, in
 *   bytes.
 */
template <std::size_t wire_size_value>
class fixed_string
    : public boost::totally_ordered<fixed_string<wire_size_value>>,
      public boost::totally_ordered<fixed_string<wire_size_value>,
                                    std::string> {
public:
  /// The size of the field on the wire
  constexpr static std::size_t wire_size = wire_size_value;

  /// Default constructor
  fixed_string() = default;

  /// Constructor from a std::string
  explicit fixed_string(std::string const& rhs) {
    static_assert(
        std::is_pod<fixed_string<wire_size>>::value,
        "fixed_string must be a PODType.");
    std::fill(buffer_, buffer_ + wire_size, ' ');
    std::strncpy(buffer_, rhs.c_str(), std::min(wire_size, rhs.length()));
  }

  /// Assingment from std::string
  fixed_string& operator=(std::string const& rhs) {
    std::fill(buffer_, buffer_ + wire_size, ' ');
    std::strncpy(buffer_, rhs.c_str(), std::min(wire_size, rhs.length()));
    return *this;
  }

  /// Return a representation as a std::string
  std::string str() const {
    return std::string(buffer_, wire_size);
  }

  //@{
  /**
   * @name Base comparison operators
   */
  /// compare vs another fixed_string
  bool operator==(fixed_string const& rhs) const {
    return std::strncmp(buffer_, rhs.buffer_, wire_size) == 0;
  }

  /// compare vs another fixed_string
  bool operator<(fixed_string const& rhs) const {
    return std::strncmp(buffer_, rhs.buffer_, wire_size) < 0;
  }

  /// compare vs a std::string
  bool operator==(std::string const& rhs) const {
    return std::strncmp(buffer_, rhs.c_str(), wire_size) == 0;
  }

  /// compare vs a std::string
  bool operator<(std::string const& rhs) const {
    return std::strncmp(buffer_, rhs.c_str(), wire_size) < 0;
  }
  //@}

  //@{
  /// @name default assignment and copy operators
  fixed_string(fixed_string const& rhs) = default;
  fixed_string(fixed_string&& rhs) = default;
  fixed_string& operator=(fixed_string const& rhs) = default;
  fixed_string& operator=(fixed_string&& rhs) = default;
  //@}

private:
  /// The in-memory representation
  char buffer_[wire_size];
};

/// Streaming operator for jb::mktdata::fixed_string<>
template <std::size_t size>
std::ostream& operator<<(std::ostream& os, fixed_string<size> const& x) {
  return os << x.str();
}

/// Implement a hash function and integrate with boost::hash
template <std::size_t size>
std::size_t hash_value(fixed_string<size> const& x) {
  auto tmp = x.str();
  return boost::hash_range(tmp.begin(), tmp.end());
}

/// Define the constexpr
template <std::size_t size>
constexpr std::size_t fixed_string<size>::wire_size;

} // namespace jb

#endif // jb_fixed_string_hpp
