#ifndef jb_mktdata_detail_levels_name_hpp
#define jb_mktdata_detail_levels_name_hpp

#include <jb/mktdata/timestamp.hpp>

namespace jb {
namespace mktdata {
namespace detail {

/**
 * Define the message name for a inside feed with N levels.
 *
 * JayBeams assigns a name (implemented as a 16-bit identifier) to
 * each message.  Messages that represent the top N levels of a market
 * are named differently depending on how many levels they have.  This
 * class defines the names for each supported level.
 *
 * @tparam N the number of levels.
 */
template <std::size_t N>
struct levels_name {};

/// Specialize for N == 1.
template <>
struct levels_name<1> {
  static constexpr char name = u'1';
};

/// Specialize for N == 4.
template <>
struct levels_name<4> {
  static constexpr char name = u'4';
};

/// Specialize for N == 8.
template <>
struct levels_name<8> {
  static constexpr char name = u'8';
};

} // namespace detail
} // namespace mktdata
} // namespace jb

#endif // jb_mktdata_detail_levels_name_hpp
