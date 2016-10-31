#ifndef jb_fftw_plan_hpp
#define jb_fftw_plan_hpp

#include <jb/fftw/cast.hpp>

#include <memory>
#include <stdexcept>

namespace jb {
namespace fftw {

int const default_flags = FFTW_ESTIMATE | FFTW_PRESERVE_INPUT | FFTW_UNALIGNED;

/**
 * Wrap FFTW3 plan objects to automatically destroy them.
 *
 * The FFTW3 optimizes execution by pre-computing coefficients and
 * execution plans for a DFT based on the original types, size and
 * alingment of the data.  These plans must be destroyed to release
 * resources.  FFTW3, being a C library, requires wrappers to automate
 * the destruction of these objects.
 *
 * In addition, the FFTW3 library uses different names for the types
 * that have single (fftwf_*), double (fftw_*) or quad-precision
 * (fftwl_*).  In C++ we prefer to hide such details in generics.
 *
 * @tparam precision_t the type of floating point value to use, must
 * be either float, double or long double.
 */
template <typename precision_t>
class plan {
public:
  //@{
  /**
   * type traits
   */
  typedef ::jb::fftw::traits<precision_t> traits;
  typedef typename traits::precision_type precision_type;
  typedef typename traits::std_complex_type std_complex_type;
  typedef typename traits::fftw_complex_type fftw_complex_type;
  typedef typename traits::fftw_plan_type fftw_plan_type;
  //@}

  /// Create unusable, empty, or null plan
  plan()
      : p_(nullptr) {
  }

  /// Basic move constructor.
  plan(plan&& rhs)
      : p_(rhs.p_) {
    rhs.p_ = nullptr;
  }

  /// Move assignment
  plan& operator=(plan&& rhs) {
    plan tmp(std::move(rhs));
    std::swap(p_, tmp.p_);
    return *this;
  }

  /// Destructor, cleanup the plan.
  ~plan() {
    if (p_ != nullptr) {
      traits::destroy_plan(p_);
    }
  }

  //@{
  /**
   * @name Prevent copy construction and assignment
   */
  plan(plan const&) = delete;
  plan& operator=(plan const&) = delete;
  //@}

  /// Execute the plan for vectors
  template <typename invector, typename outvector>
  void execute(invector const& in, outvector& out) {
    if (in.size() != out.size()) {
      throw std::invalid_argument("mismatched vector size in create_forward()");
    }
    execute_impl(fftw_cast(in), fftw_cast(out));
  }

  /// Create the plan for vectors
  template <typename invector, typename outvector>
  static plan create_forward(
      invector const& in, outvector& out, int flags = default_flags) {
    if (in.size() != out.size()) {
      throw std::invalid_argument("mismatched vector size in create_forward()");
    }
    return create_forward_impl(in.size(), fftw_cast(in), fftw_cast(out), flags);
  }

  /// Create the plan for vectors
  template <typename invector, typename outvector>
  static plan create_backward(
      invector const& in, outvector& out, int flags = default_flags) {
    if (in.size() != out.size()) {
      throw std::invalid_argument("mismatched vector size in create_forward()");
    }
    return create_backward_impl(
        in.size(), fftw_cast(in), fftw_cast(out), flags);
  }

private:
  /// Execute the plan in the c2c case
  void execute_impl(fftw_complex_type const* in, fftw_complex_type* out) {
    traits::execute_plan(p_, in, out);
  }

  /// Execute the plan for arrays of r2c case
  void execute_impl(precision_type const* in, fftw_complex_type* out) {
    traits::execute_plan(p_, in, out);
  }

  /// Execute the plan for arrays of r2c case
  void execute_impl(fftw_complex_type const* in, precision_type* out) {
    traits::execute_plan(p_, in, out);
  }

  /// Create the direct plan for arrays in the c2c case.
  static plan create_forward_impl(
      std::size_t nsamples, fftw_complex_type const* in, fftw_complex_type* out,
      int flags) {
    return plan(traits::create_forward_plan(nsamples, in, out, flags));
  }

  /// Create the inverse plan for arrays in the c2c case
  static plan create_backward_impl(
      std::size_t nsamples, fftw_complex_type const* in, fftw_complex_type* out,
      int flags) {
    return plan(traits::create_backward_plan(nsamples, in, out, flags));
  }

  /// Create the plan for arrays for r2c case
  static plan create_forward_impl(
      std::size_t nsamples, precision_type const* in, fftw_complex_type* out,
      int flags) {
    return plan(traits::create_plan(nsamples, in, out, flags));
  }

  /// Create the plan for the c2r case
  static plan create_backward_impl(
      std::size_t nsamples, fftw_complex_type const* in, precision_type* out,
      int flags) {
    return plan(traits::create_plan(nsamples, in, out, flags));
  }

private:
  plan(fftw_plan_type p)
      : p_(p) {
  }

private:
  fftw_plan_type p_;
};

} // namespace fftw
} // namespace jb

#endif // jb_fftw_plan_hpp
