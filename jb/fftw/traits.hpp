#ifndef jb_fftw_traits_hpp
#define jb_fftw_traits_hpp

#include <jb/fftw/traits.hpp>
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
template<typename precision_t>
struct traits;

#if 0
/**
 * Implement traits for single-precision floating point values.
 */
template<>
struct traits<float> {
  typedef float base_type;
  typedef std::complex<float> std_complex_type;
  typedef ::fftwf_complex fftw_complex_type;

  typedef ::fftwf_plan fftw_plan_type;
  static void destroy_plan(fftw_plan_type p) {
    ::fftwf_destroy_plan(p);
  }
  static void execute_plan(
      fftw_plan_type const p, fftw_complex_type const* in,
      fftw_complex_type* out) {
    ::fftwf_execute_dft(p, const_cast<fftw_complex_type*>(in), out);
  }
  static void execute_plan(
      fftw_plan_type const p, base_type const* in,
      fftw_complex_type* out) {
    ::fftwf_execute_dft_r2c(p, const_cast<base_type*>(in), out);
  }
  static void execute_plan(
      fftw_plan_type const p, fftw_complex_type const* in,
      base_type* out) {
    ::fftwf_execute_dft_c2r(p, const_cast<fftw_complex_type*>(in), out);
  }
};
#endif

/**
 * Implement traits for double-precision floating point numbers.
 */
template<>
struct traits<double> {
  typedef double base_type;
  typedef ::std::complex<double> std_complex_type;
  typedef ::fftw_complex fftw_complex_type;

  typedef ::fftw_plan fftw_plan_type;

  static void* allocate(std::size_t n) {
    return ::fftw_malloc(n);
  }
  static void release(void* buffer) {
    ::fftw_free(buffer);
  }
  static void destroy_plan(fftw_plan_type p) {
    ::fftw_destroy_plan(p);
  }
  static void execute_plan(
      fftw_plan_type const p, fftw_complex_type const* in,
      fftw_complex_type* out) {
    ::fftw_execute_dft(p, const_cast<fftw_complex_type*>(in), out);
  }
  static void execute_plan(
      fftw_plan_type const p, base_type const* in,
      fftw_complex_type* out) {
    ::fftw_execute_dft_r2c(p, const_cast<base_type*>(in), out);
  }
  static void execute_plan(
      fftw_plan_type const p, fftw_complex_type const* in,
      base_type* out) {
    ::fftw_execute_dft_c2r(p, const_cast<fftw_complex_type*>(in), out);
  }
  static fftw_plan_type create_forward_plan(
      int size, fftw_complex_type const* in, fftw_complex_type* out) {
    return ::fftw_plan_dft_1d(
        size, const_cast<fftw_complex_type*>(in), out,
        FFTW_FORWARD, FFTW_ESTIMATE);
  }
  static fftw_plan_type create_backward_plan(
      int size, fftw_complex_type const* in, fftw_complex_type* out) {
    return ::fftw_plan_dft_1d(
        size, const_cast<fftw_complex_type*>(in), out,
        FFTW_BACKWARD, FFTW_ESTIMATE);
  }
};

#if 0
/**
 * Implement traits for quad-precision floating point numbers.
 */
template<>
struct traits<long double> {
  typedef long double base_type;
  typedef std::complex<long double> std_complex;
  typedef ::fftwl_complex fftw_complex_type;

  typedef ::fftwl_plan fftw_plan_type;
  static void destroy_plan(fftw_plan_type p) {
    ::fftwl_destroy_plan(p);
  }
  static void execute_plan(
      fftw_plan_type const p, fftw_complex_type const* in,
      fftw_complex_type* out) {
    ::fftwl_execute_dft(p, in, out);
  }
  static void execute_plan(
      fftw_plan_type const p, base_type const* in,
      fftw_complex_type* out) {
    ::fftwl_execute_dft_r2c(p, in, out);
  }
  static void execute_plan(
      fftw_plan_type const p, fftw_complex_type const* in,
      base_type* out) {
    ::fftwl_execute_dft_c2r(p, in, out);
  }
};
#endif

} // namespace fftw
} // namespace jb

#endif // jb_fftw_traits_hpp
