#ifndef jb_complex_traits_hpp
#define jb_complex_traits_hpp

#include <complex>

namespace jb {

/**
 * Generic function to extract the floating point type of std::complex.
 *
 * Some of the algorithms and data structures in JayBeams need to work
 * for both std::complex<> and for primitive floating point values
 * such as 'float'.  These functions sometimes need a generic way to
 * extract the 'T' parameter in a std::complex<T>.  This function can
 * be used to perform such conversions, for example:
 *
 * @code
 * template<typename sample_t> class Foo {
 *   typedef typename jb::extra_value_type<sample_t>::precision prec;
 *   prec epsilon = std::numeric_limits<prec>::epsilon();
 * };
 * @endcode
 *
 * Such code would work whether sample_t is a std::complex<T> or a
 * primitive floating point value.
 */
template<typename T>
struct extract_value_type {
  typedef T precision;
};

/// Partial specialization of jb::extract_value_type for std::complex.
template<typename T>
struct extract_value_type<std::complex<T>> {
  typedef T precision;
};

} // namespace jb

#endif // jb_complex_traits_hpp
