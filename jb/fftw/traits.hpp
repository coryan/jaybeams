#ifndef jb_fftw_traits_hpp
#define jb_fftw_traits_hpp

#include <fftw3.h>
#include <complex>
#include <vector>

namespace jb {
namespace fftw {

/**
 * Wrap fftw_* types and operations to treat floating point values generically.
 *
 * Type traits to handle single-precision (float / fftwf_*),
 * double-precision (double / fftw_*) and quad-precision
 * (long double / fftwl_*) generically in C++ code.
 *
 * @tparam precision_t the type of floating point value, must be
 * either float, double or long double.
 */
template <typename precision_t>
struct traits;

/**
 * Implement traits for double-precision floating point numbers.
 */
template <>
struct traits<double> {
  typedef double precision_type;
  typedef ::std::complex<double> std_complex_type;
  typedef ::fftw_complex fftw_complex_type;
  typedef ::fftw_plan fftw_plan_type;

  static void* allocate(std::size_t n) {
    return ::fftw_malloc(n);
  }
  static void release(void* buffer) {
    ::fftw_free(buffer);
  }

  static void execute_plan(
      fftw_plan_type const p, fftw_complex_type const* in,
      fftw_complex_type* out) {
    ::fftw_execute_dft(p, const_cast<fftw_complex_type*>(in), out);
  }
  static void execute_plan(
      fftw_plan_type const p, precision_type const* in,
      fftw_complex_type* out) {
    ::fftw_execute_dft_r2c(p, const_cast<precision_type*>(in), out);
  }
  static void execute_plan(
      fftw_plan_type const p, fftw_complex_type const* in,
      precision_type* out) {
    ::fftw_execute_dft_c2r(p, const_cast<fftw_complex_type*>(in), out);
  }

  static fftw_plan_type create_forward_plan(
      std::size_t size, fftw_complex_type const* in, fftw_complex_type* out,
      int flags) {
    return ::fftw_plan_dft_1d(
        size, const_cast<fftw_complex_type*>(in), out, FFTW_FORWARD, flags);
  }
  static fftw_plan_type create_backward_plan(
      std::size_t size, fftw_complex_type const* in, fftw_complex_type* out,
      int flags) {
    return ::fftw_plan_dft_1d(
        size, const_cast<fftw_complex_type*>(in), out, FFTW_BACKWARD, flags);
  }

  static fftw_plan_type create_plan(
      std::size_t size, precision_type const* in, fftw_complex_type* out,
      int flags) {
    return ::fftw_plan_dft_r2c_1d(
        size, const_cast<precision_type*>(in), out, flags);
  }
  static fftw_plan_type create_plan(
      std::size_t size, fftw_complex_type const* in, precision_type* out,
      int flags) {
    return ::fftw_plan_dft_c2r_1d(
        size, const_cast<fftw_complex_type*>(in), out, flags);
  }

  static void destroy_plan(fftw_plan_type p) {
    ::fftw_destroy_plan(p);
  }
};

/**
 * Implement traits for single-precision floating point values.
 */
template <>
struct traits<float> {
  typedef float precision_type;
  typedef std::complex<float> std_complex_type;
  typedef ::fftwf_complex fftw_complex_type;
  typedef ::fftwf_plan fftw_plan_type;

  static void* allocate(std::size_t n) {
    return ::fftwf_malloc(n);
  }
  static void release(void* buffer) {
    ::fftwf_free(buffer);
  }

  static void execute_plan(
      fftw_plan_type const p, fftw_complex_type const* in,
      fftw_complex_type* out) {
    ::fftwf_execute_dft(p, const_cast<fftw_complex_type*>(in), out);
  }
  static void execute_plan(
      fftw_plan_type const p, precision_type const* in,
      fftw_complex_type* out) {
    ::fftwf_execute_dft_r2c(p, const_cast<precision_type*>(in), out);
  }
  static void execute_plan(
      fftw_plan_type const p, fftw_complex_type const* in,
      precision_type* out) {
    ::fftwf_execute_dft_c2r(p, const_cast<fftw_complex_type*>(in), out);
  }

  static fftw_plan_type create_forward_plan(
      std::size_t size, fftw_complex_type const* in, fftw_complex_type* out,
      int flags) {
    return ::fftwf_plan_dft_1d(
        size, const_cast<fftw_complex_type*>(in), out, FFTW_FORWARD, flags);
  }
  static fftw_plan_type create_backward_plan(
      std::size_t size, fftw_complex_type const* in, fftw_complex_type* out,
      int flags) {
    return ::fftwf_plan_dft_1d(
        size, const_cast<fftw_complex_type*>(in), out, FFTW_BACKWARD, flags);
  }

  static fftw_plan_type create_plan(
      std::size_t size, precision_type const* in, fftw_complex_type* out,
      int flags) {
    return ::fftwf_plan_dft_r2c_1d(
        size, const_cast<precision_type*>(in), out, flags);
  }
  static fftw_plan_type create_plan(
      std::size_t size, fftw_complex_type const* in, precision_type* out,
      int flags) {
    return ::fftwf_plan_dft_c2r_1d(
        size, const_cast<fftw_complex_type*>(in), out, flags);
  }

  static void destroy_plan(fftw_plan_type p) {
    ::fftwf_destroy_plan(p);
  }
};

/**
 * Implement traits for quad-precision floating point numbers.
 */
template <>
struct traits<long double> {
  typedef long double precision_type;
  typedef ::std::complex<double> std_complex_type;
  typedef ::fftwl_complex fftw_complex_type;
  typedef ::fftwl_plan fftw_plan_type;

  static void* allocate(std::size_t n) {
    return ::fftwl_malloc(n);
  }
  static void release(void* buffer) {
    ::fftwl_free(buffer);
  }

  static void execute_plan(
      fftw_plan_type const p, fftw_complex_type const* in,
      fftw_complex_type* out) {
    ::fftwl_execute_dft(p, const_cast<fftw_complex_type*>(in), out);
  }
  static void execute_plan(
      fftw_plan_type const p, precision_type const* in,
      fftw_complex_type* out) {
    ::fftwl_execute_dft_r2c(p, const_cast<precision_type*>(in), out);
  }
  static void execute_plan(
      fftw_plan_type const p, fftw_complex_type const* in,
      precision_type* out) {
    ::fftwl_execute_dft_c2r(p, const_cast<fftw_complex_type*>(in), out);
  }

  static fftw_plan_type create_forward_plan(
      std::size_t size, fftw_complex_type const* in, fftw_complex_type* out,
      int flags) {
    return ::fftwl_plan_dft_1d(
        size, const_cast<fftw_complex_type*>(in), out, FFTW_FORWARD, flags);
  }
  static fftw_plan_type create_backward_plan(
      std::size_t size, fftw_complex_type const* in, fftw_complex_type* out,
      int flags) {
    return ::fftwl_plan_dft_1d(
        size, const_cast<fftw_complex_type*>(in), out, FFTW_BACKWARD, flags);
  }

  static fftw_plan_type create_plan(
      std::size_t size, precision_type const* in, fftw_complex_type* out,
      int flags) {
    return ::fftwl_plan_dft_r2c_1d(
        size, const_cast<precision_type*>(in), out, flags);
  }
  static fftw_plan_type create_plan(
      std::size_t size, fftw_complex_type const* in, precision_type* out,
      int flags) {
    return ::fftwl_plan_dft_c2r_1d(
        size, const_cast<fftw_complex_type*>(in), out, flags);
  }

  static void destroy_plan(fftw_plan_type p) {
    ::fftwl_destroy_plan(p);
  }
};

/**
 * Define traits for std::complex<T> in terms of the traits for T.
 *
 * The traits for a std::complex type are the same as the traits for the
 * underlying precision of the complex type instantiation.  The
 * objective is to simplify/normalize the interface into FFTW3, so all
 * uses of FFTW, either via 
 */
template<typename T>
struct traits<std::complex<T>> : public traits<T> {
};

} // namespace fftw
} // namespace jb

#endif // jb_fftw_traits_hpp
