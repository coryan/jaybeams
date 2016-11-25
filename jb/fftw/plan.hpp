#ifndef jb_fftw_plan_hpp
#define jb_fftw_plan_hpp

#include <jb/fftw/cast.hpp>
#include <jb/complex_traits.hpp>

#include <memory>
#include <stdexcept>

namespace jb {
namespace fftw {

int constexpr default_plan_flags =
    (FFTW_ESTIMATE | FFTW_PRESERVE_INPUT | FFTW_UNALIGNED);

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
 * @tparam in_timeseries_type the type of the input timeseries
 * @tparam out_timeseries_type the type of the output timeseries
 */
template <typename in_timeseries_type, typename out_timeseries_type>
class plan {
public:
  //@{
  /**
   * type traits
   */
  typedef typename in_timeseries_type::value_type in_value_type;
  typedef typename jb::extract_value_type<in_value_type>::precision precision_type;
  typedef ::jb::fftw::traits<precision_type> traits;
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
    check_constraints checker;
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
  void execute(in_timeseries_type const& in, out_timeseries_type& out) {
    if (in.size() != out.size()) {
      throw std::invalid_argument("mismatched vector size in execute()");
    }
    execute_impl(fftw_cast(in), fftw_cast(out));
  }

private:
  // forward declare a helper type to check compile-time constraints.
  struct check_constraints;

  // grant access to create_*_impl functions
  template <typename itype, typename otype>
  friend plan<itype, otype>
  create_forward_plan(itype const&, otype&, int);
  template <typename itype, typename otype>
  friend plan<itype, otype>
  create_backward_plan(itype const&, otype&, int);

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

/**
 * Create a plan to compute the DFT given the input and output
 * vectors.
 *
 * @param in the input timeseries
 * @param out the output timeseries
 * @param flags the FFTW flags for the plan
 * @throws std::invalid_argument if the input and output vectors are
 * not compatible
 * @returns a jb::fftw::plan<> of the right type.
 *
 * @tparam in_timeseries_type the type of the input timeseries
 * @tparam out_timeseries_type the type of the output timeseries
 */
template <typename in_timeseries_type, typename out_timeseries_type>
plan<in_timeseries_type, out_timeseries_type> create_forward_plan(
    in_timeseries_type const& in, out_timeseries_type& out,
    int flags = default_plan_flags) {
  if (in.size() != out.size()) {
    throw std::invalid_argument(
        "mismatched vector size in create_forward_plan()");
  }
  return plan<in_timeseries_type, out_timeseries_type>::create_forward_impl(
      in.size(), fftw_cast(in), fftw_cast(out), flags);
}

/**
 * Create a plan to compute the inverse DFT given the input and output
 * vectors.
 *
 * @param in the input timeseries
 * @param out the output timeseries
 * @param flags the FFTW flags for the plan
 * @throws std::invalid_argument if the input and output vectors are
 * not compatible
 * @returns a jb::fftw::plan<> of the right type.
 *
 * @tparam in_timeseries_type the type of the input timeseries
 * @tparam out_timeseries_type the type of the output timeseries
 */
template <typename in_timeseries_type, typename out_timeseries_type>
plan<in_timeseries_type, out_timeseries_type> create_backward_plan(
    in_timeseries_type const& in, out_timeseries_type& out,
    int flags = default_plan_flags) {
  if (in.size() != out.size()) {
    throw std::invalid_argument(
        "mismatched vector size in create_backward_plan()");
  }
  return plan<in_timeseries_type, out_timeseries_type>::create_backward_impl(
      in.size(), fftw_cast(in), fftw_cast(out), flags);
}

/**
 * Check the compile-time constraints for a jb::fftw::plan<>
 */
template <typename in_timeseries_type, typename out_timeseries_type>
struct plan<in_timeseries_type, out_timeseries_type>::check_constraints {
  check_constraints() {
    typedef typename in_timeseries_type::value_type in_value_type;
    typedef typename out_timeseries_type::value_type out_value_type;
    typedef typename jb::extract_value_type<in_value_type>::precision in_precision_type;
    typedef typename jb::extract_value_type<out_value_type>::precision out_precision_type;
    static_assert(
        std::is_same<in_precision_type, out_precision_type>::value,
        "Mismatched precision_type, both timeseries must have the same"
        " precision");
  }
};

} // namespace fftw
} // namespace jb

#endif // jb_fftw_plan_hpp
