#ifndef jb_clfft_complex_traits_hpp
#define jb_clfft_complex_traits_hpp

#include <jb/clfft/init.hpp>

#include <complex>

namespace jb {
namespace clfft {
namespace detail {

/**
 * Generic version, not implemented.
 */
template <typename T>
struct complex_traits {};

/**
 * Define traits for std::complex<float> as needed in jb::clfft::plan
 */
template <>
struct complex_traits<std::complex<float>> {
  static clfftPrecision constexpr precision = CLFFT_SINGLE;
  static clfftLayout constexpr layout = CLFFT_COMPLEX_INTERLEAVED;
};

/**
 * Define traits for std::complex<double> as needed in jb::clfft::plan
 */
template <>
struct complex_traits<std::complex<double>> {
  static clfftPrecision constexpr precision = CLFFT_DOUBLE;
  static clfftLayout constexpr layout = CLFFT_COMPLEX_INTERLEAVED;
};

/**
 * Define traits for float as needed in jb::clfft::plan
 */
template <>
struct complex_traits<float> {
  static clfftPrecision constexpr precision = CLFFT_SINGLE;
  static clfftLayout constexpr layout = CLFFT_REAL;
};

/**
 * Define traits for double as needed in jb::clfft::plan
 */
template <>
struct complex_traits<double> {
  static clfftPrecision constexpr precision = CLFFT_DOUBLE;
  static clfftLayout constexpr layout = CLFFT_REAL;
};

} // namespace detail
} // namespace clfft
} // namespace jb

#endif // jb_clfft_complex_traits_hpp
