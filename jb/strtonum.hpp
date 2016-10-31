#ifndef jb_strtonum_hpp
#define jb_strtonum_hpp

#include <stdexcept>
#include <string>

namespace jb {

/**
 * Traits to convert strings to numbers (e.g. integers, floats, etc).
 *
 * For most types this refactors repetitive code, but also allows
 * specialization for specific types.
 */
template <typename T>
struct stn_traits;

#define STN_TRAITS(T, F)                                                       \
  template <>                                                                  \
  struct stn_traits<T> {                                                       \
    static T stot(std::string const& s, std::size_t& end) {                    \
      return (F)(s, &end);                                                     \
    }                                                                          \
  }

STN_TRAITS(int, std::stoi);
STN_TRAITS(unsigned long long, std::stoull);
STN_TRAITS(long long, std::stoll);
STN_TRAITS(unsigned long, std::stoul);
STN_TRAITS(long, std::stol);
STN_TRAITS(float, std::stof);
STN_TRAITS(double, std::stod);

#undef STN_TRAITS

/**
 * Generic string to number conversion with validation.
 */
template <typename T>
bool strtonum(std::string const& s, T& r) {
  if (s.empty()) {
    return false;
  }
  try {
    std::size_t end;
    T tmp = stn_traits<T>::stot(s, end);
    if (end != s.length()) {
      return false;
    }
    r = tmp;
  } catch (std::exception const&) {
    return false;
  }
  return true;
}

} // namespace jb

#endif // jb_strtonum_hpp
