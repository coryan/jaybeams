#ifndef jb_fftw_plan_hpp
#define jb_fftw_plan_hpp

#include <jb/detail/array_traits.hpp>
#include <jb/fftw/cast.hpp>
#include <jb/complex_traits.hpp>

#include <memory>

namespace jb {
namespace fftw {

int constexpr default_plan_flags =
    (FFTW_ESTIMATE | FFTW_PRESERVE_INPUT | FFTW_UNALIGNED);

namespace detail {
/**
 * Helper function to check the inputs to a create_*_plan_*()
 *
 * @param in_elements the number of elements in the input vector or array
 * @param on_elements the number of elements in the output vector or array
 * @param in_nsamples the number of samples per-timeseries in the input array
 * @param on_nsamples the number of samples per timeseries in the output array
 * @param function_name the name of the function to include in the
 * exception message if a problem is detected
 * @throws std::invalid_argument if the sizes do not match, with a
 * descriptive message.
 */
void check_plan_inputs(
    std::size_t in_elements, std::size_t on_elements, std::size_t in_nsamples,
    std::size_t on_nsamples, char const* function_name);
} // namespace detail

/**
 * Wrap FFTW3 plan objects.
 *
 * The FFTW3 optimizes execution by pre-computing coefficients and
 * execution plans for a DFT based on the original types, size and
 * alingment of the data.  In C++, we prefer the type system to
 * remember details like this instead of getting an error message when
 * we use the wrong types (or a crash).
 *
 * In addition, the FFTW3 library uses different names for the types
 * that have single (fftwf_*), double (fftw_*) or quad-precision
 * (fftwl_*).  In C++ we prefer to hide such details in generics.
 *
 * Finally, these plans must be destroyed to release resources.
 * FFTW3, being a C library, requires wrappers to automate the
 * destruction of these objects.
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
  using in_value_type =
      typename jb::detail::array_traits<in_timeseries_type>::element_type;
  using precision_type =
      typename jb::extract_value_type<in_value_type>::precision;
  using traits = ::jb::fftw::traits<precision_type>;
  using std_complex_type = typename traits::std_complex_type;
  using fftw_complex_type = typename traits::fftw_complex_type;
  using fftw_plan_type = typename traits::fftw_plan_type;
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
  void execute(in_timeseries_type const& in, out_timeseries_type& out) const {
    using namespace detail;
    check_plan_inputs(
        jb::detail::element_count(in), jb::detail::element_count(out),
        jb::detail::nsamples(in), jb::detail::nsamples(out), __func__);
    execute_impl(fftw_cast(in), fftw_cast(out));
  }

private:
  // forward declare a helper type to check compile-time constraints.
  struct check_constraints;

  // grant access to create_*_impl functions
  template <typename itype, typename otype>
  friend plan<itype, otype> create_forward_plan(itype const&, otype&, int);
  template <typename itype, typename otype>
  friend plan<itype, otype> create_backward_plan(itype const&, otype&, int);
  template <typename itype, typename otype>
  friend plan<itype, otype> create_forward_plan_1d(itype const&, otype&, int);
  template <typename itype, typename otype>
  friend plan<itype, otype> create_backward_plan_1d(itype const&, otype&, int);

private:
  /// Execute the plan in the c2c case
  void execute_impl(fftw_complex_type const* in, fftw_complex_type* out) const {
    traits::execute_plan(p_, in, out);
  }

  /// Execute the plan for arrays of r2c case
  void execute_impl(precision_type const* in, fftw_complex_type* out) const {
    traits::execute_plan(p_, in, out);
  }

  /// Execute the plan for arrays of r2c case
  void execute_impl(fftw_complex_type const* in, precision_type* out) const {
    traits::execute_plan(p_, in, out);
  }

  /// Create the direct plan for vectors in the c2c case.
  static plan create_forward_impl(
      std::size_t nsamples, fftw_complex_type const* in, fftw_complex_type* out,
      int flags) {
    return plan(traits::create_forward_plan(nsamples, in, out, flags));
  }

  /// Create the inverse plan for vectors in the c2c case
  static plan create_backward_impl(
      std::size_t nsamples, fftw_complex_type const* in, fftw_complex_type* out,
      int flags) {
    return plan(traits::create_backward_plan(nsamples, in, out, flags));
  }

  /// Create the plan for vectors in the r2c case
  static plan create_forward_impl(
      std::size_t nsamples, precision_type const* in, fftw_complex_type* out,
      int flags) {
    return plan(traits::create_plan(nsamples, in, out, flags));
  }

  /// Create the plan for vectors in the c2r case
  static plan create_backward_impl(
      std::size_t nsamples, fftw_complex_type const* in, precision_type* out,
      int flags) {
    return plan(traits::create_plan(nsamples, in, out, flags));
  }

  /// Create the direct plan for arrays in the c2c case.
  static plan create_forward_many_impl(
      int howmany, std::size_t nsamples, fftw_complex_type const* in,
      fftw_complex_type* out, int flags) {
    return plan(
        traits::create_forward_plan_many(howmany, nsamples, in, out, flags));
  }

  /// Create the inverse plan for arrays in the c2c case
  static plan create_backward_many_impl(
      int howmany, std::size_t nsamples, fftw_complex_type const* in,
      fftw_complex_type* out, int flags) {
    return plan(
        traits::create_backward_plan_many(howmany, nsamples, in, out, flags));
  }

  /// Create the batched plan for arrays in the r2c case
  static plan create_forward_many_impl(
      int howmany, std::size_t nsamples, precision_type const* in,
      fftw_complex_type* out, int flags) {
    return plan(traits::create_plan_many(howmany, nsamples, in, out, flags));
  }

  /// Create the batched plan for arrays in the c2r case
  static plan create_backward_many_impl(
      int howmany, std::size_t nsamples, fftw_complex_type const* in,
      precision_type* out, int flags) {
    return plan(traits::create_plan_many(howmany, nsamples, in, out, flags));
  }

private:
  /**
   * Constructor from a raw FFTW plan.
   *
   * @param p the raw FFTW plan
   */
  explicit plan(fftw_plan_type p)
      : p_(p) {
  }

private:
  /// The raw FFTW plan
  fftw_plan_type p_;
};

/**
 * Create a plan to compute many DFTs given the input and output
 * arrays.
 *
 * @param in the input array of timeseries
 * @param out the output array of timeseries
 * @param flags the FFTW flags for the plan
 * @throws std::invalid_argument if the input and output vectors are
 * not compatible
 * @returns a jb::fftw::plan<> of the right type.
 *
 * @tparam in_array_type the type of the input array of timeseries
 * @tparam out_array_type the type of the output array of timeseries
 */
template <typename in_array_type, typename out_array_type>
plan<in_array_type, out_array_type> create_forward_plan(
    in_array_type const& in, out_array_type& out,
    int flags = default_plan_flags) {
  using namespace detail;
  check_plan_inputs(
      jb::detail::element_count(in), jb::detail::element_count(out),
      jb::detail::nsamples(in), jb::detail::nsamples(out), __func__);
  auto howmany = jb::detail::element_count(in) / jb::detail::nsamples(in);
  if (howmany == 1) {
    return plan<in_array_type, out_array_type>::create_forward_impl(
        jb::detail::nsamples(in), fftw_cast(in), fftw_cast(out), flags);
  }
  return plan<in_array_type, out_array_type>::create_forward_many_impl(
      howmany, jb::detail::nsamples(in), fftw_cast(in), fftw_cast(out), flags);
}

/**
 * Create a plan to compute many inverse DFT given the input and output
 * arrays.
 *
 * @param in the input timeseries
 * @param out the output timeseries
 * @param flags the FFTW flags for the plan
 * @throws std::invalid_argument if the input and output vectors are
 * not compatible
 * @returns a jb::fftw::plan<> of the right type.
 *
 * @tparam in_array_type the type of the input timeseries
 * @tparam out_array_type the type of the output timeseries
 */
template <typename in_array_type, typename out_array_type>
plan<in_array_type, out_array_type> create_backward_plan(
    in_array_type const& in, out_array_type& out,
    int flags = default_plan_flags) {
  using namespace detail;
  check_plan_inputs(
      jb::detail::element_count(in), jb::detail::element_count(out),
      jb::detail::nsamples(in), jb::detail::nsamples(out), __func__);
  auto howmany = jb::detail::element_count(in) / jb::detail::nsamples(in);
  if (howmany == 1) {
    return plan<in_array_type, out_array_type>::create_backward_impl(
        jb::detail::nsamples(in), fftw_cast(in), fftw_cast(out), flags);
  }
  return plan<in_array_type, out_array_type>::create_backward_many_impl(
      howmany, jb::detail::nsamples(in), fftw_cast(in), fftw_cast(out), flags);
}

/**
 * Check the compile-time constraints for a jb::fftw::plan<>
 */
template <typename in_timeseries_type, typename out_timeseries_type>
struct plan<in_timeseries_type, out_timeseries_type>::check_constraints {
  check_constraints() {
    using in_value_type =
        typename jb::detail::array_traits<in_timeseries_type>::element_type;
    using out_value_type =
        typename jb::detail::array_traits<out_timeseries_type>::element_type;
    using in_precision_type =
        typename jb::extract_value_type<in_value_type>::precision;
    using out_precision_type =
        typename jb::extract_value_type<out_value_type>::precision;
    static_assert(
        std::is_same<in_precision_type, out_precision_type>::value,
        "Mismatched precision_type, both timeseries must have the same"
        " precision");
  }
};

} // namespace fftw
} // namespace jb

#endif // jb_fftw_plan_hpp
