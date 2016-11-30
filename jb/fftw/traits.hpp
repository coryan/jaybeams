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
 * Wrap the FFTW types and functions for double precision float point numbers
 */
template <>
struct traits<double> {
  //@{
  /**
   * @name type traits
   */
  /// The type used to represent floating point numbers
  typedef double precision_type;
  /// The type used to represent complex numbers in the C++ standard library
  typedef ::std::complex<double> std_complex_type;
  /// The type used to represent complex numbers in FFTW
  typedef ::fftw_complex fftw_complex_type;
  /// The type used to represent execution plans in FFTW
  typedef ::fftw_plan fftw_plan_type;
  //@}

  /**
   * Allocate a properly aligned (for SIMD acceleration) block of memory
   *
   * @param n the number of bytes to allocate
   * @returns a block for at least @a n bytes
   */
  static void* allocate(std::size_t n) {
    return ::fftw_malloc(n);
  }

  /**
   * Release a block of memory allocated with allocate()
   *
   * @param buffer the block to release
   */
  static void release(void* buffer) {
    ::fftw_free(buffer);
  }

  /**
   * Execute an existing plan for a given input and output
   *
   * FFTW requires, but does not check that the input and output have
   * the same alignment and sizes defined in the original plan.
   *
   * @param p the execution plan
   * @param in the input vector (or array)
   * @param out the output data (or array)
   */
  static void execute_plan(
      fftw_plan_type const p, fftw_complex_type const* in,
      fftw_complex_type* out) {
    ::fftw_execute_dft(p, const_cast<fftw_complex_type*>(in), out);
  }

  /**
   * Execute an existing plan for a given input and output
   *
   * FFTW requires, but does not check that the input and output have
   * the same alignment and sizes defined in the original plan.
   *
   * @param p the execution plan
   * @param in the input vector (or array)
   * @param out the output vector (or array)
   */
  static void execute_plan(
      fftw_plan_type const p, precision_type const* in,
      fftw_complex_type* out) {
    ::fftw_execute_dft_r2c(p, const_cast<precision_type*>(in), out);
  }

  /**
   * Execute an existing plan for a given input and output
   *
   * FFTW requires, but does not check that the input and output have
   * the same alignment and sizes defined in the original plan.
   *
   * @param p the execution plan
   * @param in the input vector (or array)
   * @param out the output vector (or array), can be the same as the input
   */
  static void execute_plan(
      fftw_plan_type const p, fftw_complex_type const* in,
      precision_type* out) {
    ::fftw_execute_dft_c2r(p, const_cast<fftw_complex_type*>(in), out);
  }

  /**
   * Create an execution plan to compute the DFT based on the input
   * and output exemplars
   *
   * @returns the execution plan
   * @param size the size of the input and output vectors
   * @param in the input vector
   * @param out the output vector
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_forward_plan(
      std::size_t size, fftw_complex_type const* in, fftw_complex_type* out,
      int flags) {
    return ::fftw_plan_dft_1d(
        size, const_cast<fftw_complex_type*>(in), out, FFTW_FORWARD, flags);
  }

  /**
   * Create an execution to compute the inverse DFT based on the input
   * and output exemplars
   *
   * @returns the execution plan
   * @param size the size of the input and output vectors
   * @param in the input vector
   * @param out the output vector
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_backward_plan(
      std::size_t size, fftw_complex_type const* in, fftw_complex_type* out,
      int flags) {
    return ::fftw_plan_dft_1d(
        size, const_cast<fftw_complex_type*>(in), out, FFTW_BACKWARD, flags);
  }

  /**
   * Create an execution to compute the DFT based on the input
   * and output exemplars
   *
   * @returns the execution plan
   * @param size the size of the input and output vectors
   * @param in the input vector
   * @param out the output vector
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_plan(
      std::size_t size, precision_type const* in, fftw_complex_type* out,
      int flags) {
    return ::fftw_plan_dft_r2c_1d(
        size, const_cast<precision_type*>(in), out, flags);
  }

  /**
   * Create an execution to compute the inverse DFT based on the input
   * and output exemplars
   *
   * @returns the execution plan
   * @param size the size of the input and output vectors
   * @param in the input vector
   * @param out the output vector
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_plan(
      std::size_t size, fftw_complex_type const* in, precision_type* out,
      int flags) {
    return ::fftw_plan_dft_c2r_1d(
        size, const_cast<fftw_complex_type*>(in), out, flags);
  }

  /**
   * Create an execution to compute the DFT of many vectors based on the input
   * and output exemplars
   *
   * @returns the execution plan
   * @param howmany how many timeseries in the arrays
   * @param size the size of the input and output vectors
   * @param in the input array must be of size @a howmany*size
   * @param out the output array must be of size @a howmany*size
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_forward_plan_many(
      int howmany, std::size_t size, fftw_complex_type const* in,
      fftw_complex_type* out, int flags) {
    int const rank = 1;
    int const n[rank] = {static_cast<int>(size)};
    int const istride = 1;
    int const ostride = 1;
    int const* inembed = nullptr;
    int const* onembed = nullptr;
    int const idist = n[0];
    int const odist = n[0];
    return ::fftw_plan_many_dft(
        rank, n, howmany, const_cast<fftw_complex_type*>(in), inembed, istride,
        idist, out, onembed, ostride, odist, FFTW_FORWARD, flags);
  }

  /**
   * Create an execution to compute the inverse DFT of many vectors
   * based on the input and output exemplars
   *
   * @returns the execution plan
   * @param howmany how many timeseries in the arrays
   * @param size the size of the input and output vectors
   * @param in the input array must be of size @a howmany*size
   * @param out the output array must be of size @a howmany*size
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_backward_plan_many(
      int howmany, std::size_t size, fftw_complex_type const* in,
      fftw_complex_type* out, int flags) {
    int const rank = 1;
    int const n[rank] = {static_cast<int>(size)};
    int const istride = 1;
    int const ostride = 1;
    int const* inembed = nullptr;
    int const* onembed = nullptr;
    int const idist = n[0];
    int const odist = n[0];
    return ::fftw_plan_many_dft(
        rank, n, howmany, const_cast<fftw_complex_type*>(in), inembed, istride,
        idist, out, onembed, ostride, odist, FFTW_BACKWARD, flags);
  }

  /**
   * Create an execution to compute the DFT of many vectors based on
   * the input and output exemplars
   *
   * @returns the execution plan
   * @param howmany how many timeseries in the arrays
   * @param size the size of the input and output vectors
   * @param in the input array must be of size @a howmany*size
   * @param out the output array must be of size @a howmany*size
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_plan_many(
      int howmany, std::size_t size, precision_type const* in,
      fftw_complex_type* out, unsigned flags) {
    int const rank = 1;
    int const n[rank] = {static_cast<int>(size)};
    int const istride = 1;
    int const ostride = 1;
    int const* inembed = nullptr;
    int const* onembed = nullptr;
    int const idist = n[0];
    int const odist = n[0];
    return ::fftw_plan_many_dft_r2c(
        rank, n, howmany, const_cast<precision_type*>(in), inembed, istride,
        idist, out, onembed, ostride, odist, flags);
  }

  /**
   * Create an execution to compute the inverse DFT of many vectors based on
   * the input and output exemplars
   *
   * @returns the execution plan
   * @param howmany how many timeseries in the arrays
   * @param size the size of the input and output vectors
   * @param in the input array must be of size @a howmany*size
   * @param out the output array must be of size @a howmany*size
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_plan_many(
      int howmany, std::size_t size, fftw_complex_type const* in,
      precision_type* out, int flags) {
    int const rank = 1;
    int const n[rank] = {static_cast<int>(size)};
    int const istride = 1;
    int const ostride = 1;
    int const* inembed = nullptr;
    int const* onembed = nullptr;
    int const idist = n[0];
    int const odist = n[0];
    return ::fftw_plan_many_dft_c2r(
        rank, n, howmany, const_cast<fftw_complex_type*>(in), inembed, istride,
        idist, out, onembed, ostride, odist, flags);
  }

  /**
   * Destroy an execution plan
   *
   * @param p the plan to destroy
   */
  static void destroy_plan(fftw_plan_type p) {
    ::fftw_destroy_plan(p);
  }
};

/**
 * Wrap the FFTW types and functions for single precision float point numbers
 */
template <>
struct traits<float> {
  //@{
  /**
   * @name type traits
   */
  /// The type used to represent floating point numbers
  typedef float precision_type;
  /// The type used to represent complex numbers in the C++ standard library
  typedef std::complex<float> std_complex_type;
  /// The type used to represent complex numbers in FFTW
  typedef ::fftwf_complex fftw_complex_type;
  /// The type used to represent execution plans in FFTW
  typedef ::fftwf_plan fftw_plan_type;
  //@}

  /**
   * Allocate a properly aligned (for SIMD acceleration) block of memory
   *
   * @param n the number of bytes to allocate
   * @returns a block for at least @a n bytes
   */
  static void* allocate(std::size_t n) {
    return ::fftwf_malloc(n);
  }

  /**
   * Release a block of memory allocated with allocate()
   *
   * @param buffer the block to release
   */
  static void release(void* buffer) {
    ::fftwf_free(buffer);
  }

  /**
   * Execute an existing plan for a given input and output
   *
   * FFTW requires, but does not check that the input and output have
   * the same alignment and sizes defined in the original plan.
   *
   * @param p the execution plan
   * @param in the input vector (or array)
   * @param out the output data (or array)
   */
  static void execute_plan(
      fftw_plan_type const p, fftw_complex_type const* in,
      fftw_complex_type* out) {
    ::fftwf_execute_dft(p, const_cast<fftw_complex_type*>(in), out);
  }

  /**
   * Execute an existing plan for a given input and output
   *
   * FFTW requires, but does not check that the input and output have
   * the same alignment and sizes defined in the original plan.
   *
   * @param p the execution plan
   * @param in the input vector (or array)
   * @param out the output vector (or array)
   */
  static void execute_plan(
      fftw_plan_type const p, precision_type const* in,
      fftw_complex_type* out) {
    ::fftwf_execute_dft_r2c(p, const_cast<precision_type*>(in), out);
  }

  /**
   * Execute an existing plan for a given input and output
   *
   * FFTW requires, but does not check that the input and output have
   * the same alignment and sizes defined in the original plan.
   *
   * @param p the execution plan
   * @param in the input vector (or array)
   * @param out the output vector (or array), can be the same as the input
   */
  static void execute_plan(
      fftw_plan_type const p, fftw_complex_type const* in,
      precision_type* out) {
    ::fftwf_execute_dft_c2r(p, const_cast<fftw_complex_type*>(in), out);
  }

  /**
   * Create an execution plan to compute the DFT based on the input
   * and output exemplars
   *
   * @returns the execution plan
   * @param size the size of the input and output vectors
   * @param in the input vector
   * @param out the output vector
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_forward_plan(
      std::size_t size, fftw_complex_type const* in, fftw_complex_type* out,
      int flags) {
    return ::fftwf_plan_dft_1d(
        size, const_cast<fftw_complex_type*>(in), out, FFTW_FORWARD, flags);
  }

  /**
   * Create an execution to compute the inverse DFT based on the input
   * and output exemplars
   *
   * @returns the execution plan
   * @param size the size of the input and output vectors
   * @param in the input vector
   * @param out the output vector
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_backward_plan(
      std::size_t size, fftw_complex_type const* in, fftw_complex_type* out,
      int flags) {
    return ::fftwf_plan_dft_1d(
        size, const_cast<fftw_complex_type*>(in), out, FFTW_BACKWARD, flags);
  }

  /**
   * Create an execution to compute the DFT based on the input
   * and output exemplars
   *
   * @returns the execution plan
   * @param size the size of the input and output vectors
   * @param in the input vector
   * @param out the output vector
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_plan(
      std::size_t size, precision_type const* in, fftw_complex_type* out,
      int flags) {
    return ::fftwf_plan_dft_r2c_1d(
        size, const_cast<precision_type*>(in), out, flags);
  }

  /**
   * Create an execution to compute the inverse DFT based on the input
   * and output exemplars
   *
   * @returns the execution plan
   * @param size the size of the input and output vectors
   * @param in the input vector
   * @param out the output vector
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_plan(
      std::size_t size, fftw_complex_type const* in, precision_type* out,
      int flags) {
    return ::fftwf_plan_dft_c2r_1d(
        size, const_cast<fftw_complex_type*>(in), out, flags);
  }

  /**
   * Create an execution to compute the DFT of many vectors based on the input
   * and output exemplars
   *
   * @returns the execution plan
   * @param howmany how many timeseries in the arrays
   * @param size the size of the input and output vectors
   * @param in the input array must be of size @a howmany*size
   * @param out the output array must be of size @a howmany*size
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_forward_plan_many(
      int howmany, std::size_t size, fftw_complex_type const* in,
      fftw_complex_type* out, int flags) {
    int const rank = 1;
    int const n[rank] = {static_cast<int>(size)};
    int const istride = 1;
    int const ostride = 1;
    int const* inembed = nullptr;
    int const* onembed = nullptr;
    int const idist = n[0];
    int const odist = n[0];
    return ::fftwf_plan_many_dft(
        rank, n, howmany, const_cast<fftw_complex_type*>(in), inembed, istride,
        idist, out, onembed, ostride, odist, FFTW_FORWARD, flags);
  }

  /**
   * Create an execution to compute the inverse DFT of many vectors
   * based on the input and output exemplars
   *
   * @returns the execution plan
   * @param howmany how many timeseries in the arrays
   * @param size the size of the input and output vectors
   * @param in the input array must be of size @a howmany*size
   * @param out the output array must be of size @a howmany*size
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_backward_plan_many(
      int howmany, std::size_t size, fftw_complex_type const* in,
      fftw_complex_type* out, int flags) {
    int const rank = 1;
    int const n[rank] = {static_cast<int>(size)};
    int const istride = 1;
    int const ostride = 1;
    int const* inembed = nullptr;
    int const* onembed = nullptr;
    int const idist = n[0];
    int const odist = n[0];
    return ::fftwf_plan_many_dft(
        rank, n, howmany, const_cast<fftw_complex_type*>(in), inembed, istride,
        idist, out, onembed, ostride, odist, FFTW_BACKWARD, flags);
  }

  /**
   * Create an execution to compute the DFT of many vectors based on
   * the input and output exemplars
   *
   * @returns the execution plan
   * @param howmany how many timeseries in the arrays
   * @param size the size of the input and output vectors
   * @param in the input array must be of size @a howmany*size
   * @param out the output array must be of size @a howmany*size
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_plan_many(
      int howmany, std::size_t size, precision_type const* in,
      fftw_complex_type* out, unsigned flags) {
    int const rank = 1;
    int const n[rank] = {static_cast<int>(size)};
    int const istride = 1;
    int const ostride = 1;
    int const* inembed = nullptr;
    int const* onembed = nullptr;
    int const idist = n[0];
    int const odist = n[0];
    return ::fftwf_plan_many_dft_r2c(
        rank, n, howmany, const_cast<precision_type*>(in), inembed, istride,
        idist, out, onembed, ostride, odist, flags);
  }

  /**
   * Create an execution to compute the inverse DFT of many vectors based on
   * the input and output exemplars
   *
   * @returns the execution plan
   * @param howmany how many timeseries in the arrays
   * @param size the size of the input and output vectors
   * @param in the input array must be of size @a howmany*size
   * @param out the output array must be of size @a howmany*size
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_plan_many(
      int howmany, std::size_t size, fftw_complex_type const* in,
      precision_type* out, int flags) {
    int const rank = 1;
    int const n[rank] = {static_cast<int>(size)};
    int const istride = 1;
    int const ostride = 1;
    int const* inembed = nullptr;
    int const* onembed = nullptr;
    int const idist = n[0];
    int const odist = n[0];
    return ::fftwf_plan_many_dft_c2r(
        rank, n, howmany, const_cast<fftw_complex_type*>(in), inembed, istride,
        idist, out, onembed, ostride, odist, flags);
  }

  /**
   * Destroy an execution plan
   *
   * @param p the plan to destroy
   */
  static void destroy_plan(fftw_plan_type p) {
    ::fftwf_destroy_plan(p);
  }
};

/**
 * Wrap the FFTW types and functions for quad precision float point numbers
 */
template <>
struct traits<long double> {
  //@{
  /**
   * @name type traits
   */
  /// The type used to represent floating point numbers
  typedef long double precision_type;
  /// The type used to represent complex numbers in the C++ standard library
  typedef ::std::complex<double> std_complex_type;
  /// The type used to represent complex numbers in FFTW
  typedef ::fftwl_complex fftw_complex_type;
  /// The type used to represent execution plans in FFTW
  typedef ::fftwl_plan fftw_plan_type;

  //@}

  /**
   * Allocate a properly aligned (for SIMD acceleration) block of memory
   *
   * @param n the number of bytes to allocate
   * @returns a block for at least @a n bytes
   */
  static void* allocate(std::size_t n) {
    return ::fftwl_malloc(n);
  }

  /**
   * Release a block of memory allocated with allocate()
   *
   * @param buffer the block to release
   */
  static void release(void* buffer) {
    ::fftwl_free(buffer);
  }

  /**
   * Execute an existing plan for a given input and output
   *
   * FFTW requires, but does not check that the input and output have
   * the same alignment and sizes defined in the original plan.
   *
   * @param p the execution plan
   * @param in the input vector (or array)
   * @param out the output data (or array)
   */
  static void execute_plan(
      fftw_plan_type const p, fftw_complex_type const* in,
      fftw_complex_type* out) {
    ::fftwl_execute_dft(p, const_cast<fftw_complex_type*>(in), out);
  }

  /**
   * Execute an existing plan for a given input and output
   *
   * FFTW requires, but does not check that the input and output have
   * the same alignment and sizes defined in the original plan.
   *
   * @param p the execution plan
   * @param in the input vector (or array)
   * @param out the output vector (or array)
   */
  static void execute_plan(
      fftw_plan_type const p, precision_type const* in,
      fftw_complex_type* out) {
    ::fftwl_execute_dft_r2c(p, const_cast<precision_type*>(in), out);
  }

  /**
   * Execute an existing plan for a given input and output
   *
   * FFTW requires, but does not check that the input and output have
   * the same alignment and sizes defined in the original plan.
   *
   * @param p the execution plan
   * @param in the input vector (or array)
   * @param out the output vector (or array), can be the same as the input
   */
  static void execute_plan(
      fftw_plan_type const p, fftw_complex_type const* in,
      precision_type* out) {
    ::fftwl_execute_dft_c2r(p, const_cast<fftw_complex_type*>(in), out);
  }

  /**
   * Create an execution plan to compute the DFT based on the input
   * and output exemplars
   *
   * @returns the execution plan
   * @param size the size of the input and output vectors
   * @param in the input vector
   * @param out the output vector
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_forward_plan(
      std::size_t size, fftw_complex_type const* in, fftw_complex_type* out,
      int flags) {
    return ::fftwl_plan_dft_1d(
        size, const_cast<fftw_complex_type*>(in), out, FFTW_FORWARD, flags);
  }

  /**
   * Create an execution to compute the inverse DFT based on the input
   * and output exemplars
   *
   * @returns the execution plan
   * @param size the size of the input and output vectors
   * @param in the input vector
   * @param out the output vector
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_backward_plan(
      std::size_t size, fftw_complex_type const* in, fftw_complex_type* out,
      int flags) {
    return ::fftwl_plan_dft_1d(
        size, const_cast<fftw_complex_type*>(in), out, FFTW_BACKWARD, flags);
  }

  /**
   * Create an execution to compute the DFT based on the input
   * and output exemplars
   *
   * @returns the execution plan
   * @param size the size of the input and output vectors
   * @param in the input vector
   * @param out the output vector
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_plan(
      std::size_t size, precision_type const* in, fftw_complex_type* out,
      int flags) {
    return ::fftwl_plan_dft_r2c_1d(
        size, const_cast<precision_type*>(in), out, flags);
  }

  /**
   * Create an execution to compute the inverse DFT based on the input
   * and output exemplars
   *
   * @returns the execution plan
   * @param size the size of the input and output vectors
   * @param in the input vector
   * @param out the output vector
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_plan(
      std::size_t size, fftw_complex_type const* in, precision_type* out,
      int flags) {
    return ::fftwl_plan_dft_c2r_1d(
        size, const_cast<fftw_complex_type*>(in), out, flags);
  }

  /**
   * Create an execution to compute the DFT of many vectors based on the input
   * and output exemplars
   *
   * @returns the execution plan
   * @param howmany how many timeseries in the arrays
   * @param size the size of the input and output vectors
   * @param in the input array must be of size @a howmany*size
   * @param out the output array must be of size @a howmany*size
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_forward_plan_many(
      int howmany, std::size_t size, fftw_complex_type const* in,
      fftw_complex_type* out, int flags) {
    int const rank = 1;
    int const n[rank] = {static_cast<int>(size)};
    int const istride = 1;
    int const ostride = 1;
    int const* inembed = nullptr;
    int const* onembed = nullptr;
    int const idist = n[0];
    int const odist = n[0];
    return ::fftwl_plan_many_dft(
        rank, n, howmany, const_cast<fftw_complex_type*>(in), inembed, istride,
        idist, out, onembed, ostride, odist, FFTW_FORWARD, flags);
  }

  /**
   * Create an execution to compute the inverse DFT of many vectors
   * based on the input and output exemplars
   *
   * @returns the execution plan
   * @param howmany how many timeseries in the arrays
   * @param size the size of the input and output vectors
   * @param in the input array must be of size @a howmany*size
   * @param out the output array must be of size @a howmany*size
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_backward_plan_many(
      int howmany, std::size_t size, fftw_complex_type const* in,
      fftw_complex_type* out, int flags) {
    int const rank = 1;
    int const n[rank] = {static_cast<int>(size)};
    int const istride = 1;
    int const ostride = 1;
    int const* inembed = nullptr;
    int const* onembed = nullptr;
    int const idist = n[0];
    int const odist = n[0];
    return ::fftwl_plan_many_dft(
        rank, n, howmany, const_cast<fftw_complex_type*>(in), inembed, istride,
        idist, out, onembed, ostride, odist, FFTW_BACKWARD, flags);
  }

  /**
   * Create an execution to compute the DFT of many vectors based on
   * the input and output exemplars
   *
   * @returns the execution plan
   * @param howmany how many timeseries in the arrays
   * @param size the size of the input and output vectors
   * @param in the input array must be of size @a howmany*size
   * @param out the output array must be of size @a howmany*size
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_plan_many(
      int howmany, std::size_t size, precision_type const* in,
      fftw_complex_type* out, unsigned flags) {
    int const rank = 1;
    int const n[rank] = {static_cast<int>(size)};
    int const istride = 1;
    int const ostride = 1;
    int const* inembed = nullptr;
    int const* onembed = nullptr;
    int const idist = n[0];
    int const odist = n[0];
    return ::fftwl_plan_many_dft_r2c(
        rank, n, howmany, const_cast<precision_type*>(in), inembed, istride,
        idist, out, onembed, ostride, odist, flags);
  }

  /**
   * Create an execution to compute the inverse DFT of many vectors based on
   * the input and output exemplars
   *
   * @returns the execution plan
   * @param howmany how many timeseries in the arrays
   * @param size the size of the input and output vectors
   * @param in the input array must be of size @a howmany*size
   * @param out the output array must be of size @a howmany*size
   * @param flags control the algorithm choices in FFTW
   */
  static fftw_plan_type create_plan_many(
      int howmany, std::size_t size, fftw_complex_type const* in,
      precision_type* out, int flags) {
    int const rank = 1;
    int const n[rank] = {static_cast<int>(size)};
    int const istride = 1;
    int const ostride = 1;
    int const* inembed = nullptr;
    int const* onembed = nullptr;
    int const idist = n[0];
    int const odist = n[0];
    return ::fftwl_plan_many_dft_c2r(
        rank, n, howmany, const_cast<fftw_complex_type*>(in), inembed, istride,
        idist, out, onembed, ostride, odist, flags);
  }

  /**
   * Destroy an execution plan
   *
   * @param p the plan to destroy
   */
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
 * uses of FFTW look similar in C++.
 */
template <typename T>
struct traits<std::complex<T>> : public traits<T> {};

} // namespace fftw
} // namespace jb

#endif // jb_fftw_traits_hpp
