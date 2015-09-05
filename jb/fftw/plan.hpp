#ifndef jb_fftw_plan_hpp
#define jb_fftw_plan_hpp

#include <jb/fftw/cast.hpp>

#include <memory>
#include <stdexcept>

namespace jb {
namespace fftw {

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
template<typename precision_t>
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
  template<typename vector>
  void execute(vector const& in, vector& out) {
    if (in.size() != out.size()) {
      throw std::invalid_argument("mismatched vector size in create_forward()");
    }
    execute_impl(fftw_cast(in), fftw_cast(out));
  }

  /// Create the plan for vectors
  template<typename vector>
  static plan create_forward(vector const& in, vector& out) {
    if (in.size() != out.size()) {
      throw std::invalid_argument("mismatched vector size in create_forward()");
    }
    return create_forward_impl(
        in.size(), fftw_cast(in), fftw_cast(out));
  }

  /// Create the plan for vectors
  template<typename vector>
  static plan create_backward(vector const& in, vector& out) {
    if (in.size() != out.size()) {
      throw std::invalid_argument("mismatched vector size in create_forward()");
    }
    return create_backward_impl(
        in.size(), fftw_cast(in), fftw_cast(out));
  }

 private:
  /// Execute the plan for arrays of fftw_complex numbers
  void execute_impl(
      fftw_complex_type const* in, fftw_complex_type* out) {
    traits::execute_plan(p_, in, out);
  }

  /// Create the plan for arrays for fftw_complex numbers
  static plan create_forward_impl(
      std::size_t nsamples, fftw_complex_type const* in,
      fftw_complex_type* out) {
    return plan(traits::create_forward_plan(nsamples, in, out));
  }

  static plan create_backward_impl(
      std::size_t nsamples, fftw_complex_type const* in,
      fftw_complex_type* out) {
    return plan(traits::create_backward_plan(nsamples, in, out));
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
