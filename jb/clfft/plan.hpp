#ifndef jb_clfft_plan_hpp
#define jb_clfft_plan_hpp

#include <jb/clfft/complex_traits.hpp>
#include <jb/clfft/error.hpp>
#include <jb/clfft/init.hpp>
#include <jb/complex_traits.hpp>

#include <boost/compute/command_queue.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/event.hpp>
#include <boost/compute/utility/wait_list.hpp>

namespace jb {
namespace clfft {
/**
 * Wrap clfftPlanHandle objects.
 *
 * The clFFT library optimizes execution by pre-computing
 * coefficients, execution plans, and OpenCL kernels for a given
 * input.  The plan also stores the precision (float vs. double), and
 * the input and output types (e.g. float to std::complex<float>
 * vs. std::complex<float> to std::complex<float>).
 *
 * In JayBeams we prefer to store such details in the type system, and
 * create a different plan type for different input and output types.
 *
 * @tparam in_timeseries_type the type of the input timeseries,
 * typically an instantiation of boost::compute::vector<T>.
 * @tparam out_timseries_type the type of the output timeseries,
 * typically an instantiation of boost::compute::vector<T>.
 */
template <typename in_timeseries_type, typename out_timeseries_type>
class plan {
public:
  //@{
  /**
   * type straits
   */
  using in_value_type = typename in_timeseries_type::value_type;
  using out_value_type = typename out_timeseries_type::value_type;
  using in_value_traits = ::jb::clfft::detail::complex_traits<in_value_type>;
  using out_value_traits = ::jb::clfft::detail::complex_traits<out_value_type>;
  // The precision type must be the same for out_value_type and
  // in_value_type, this is statically checked by
  // jb::fftw::plan::check_constraints.
  using precision_type =
      typename jb::extract_value_type<in_value_type>::precision;
  //@}

  /// Default constructor
  plan()
      : p_()
      , d_(ENDDIRECTION) {
  }

  /// Basic move constructor
  plan(plan&& rhs)
      : p_(rhs.p_)
      , d_(rhs.d_) {
    plan tmp;
    rhs.p_ = tmp.p_;
    rhs.d_ = tmp.d_;
  }

  /// Move assigment operator
  plan& operator=(plan&& rhs) {
    plan tmp(std::move(rhs));
    std::swap(p_, tmp.p_);
    std::swap(d_, tmp.d_);
    return *this;
  }

  /// Destructor, cleanup the plan.
  ~plan() {
    check_constraints checker;
    if (d_ == ENDDIRECTION) {
      return;
    }
    cl_int err = clfftDestroyPlan(&p_);
    jb::clfft::check_error_code(err, "clfftDestroyPlan");
  }

  /**
   * Enqueue the transform to be executed.
   *
   * As it is often the case with OpenCL, this is an asynchronous
   * operation.  The transform is scheduled to be executed, but it may
   * not have completed by the time this function returns.  One must
   * wait for the returned event to reach the @a done state before any
   * operations can depend on the state of the output.
   *
   * @param out the output location for the data, must have the same
   * size as the prototype used to construct the plan.
   * @param in the input for the data, must have the same size as the
   * prototype used to construct the plan.
   * @param queue where to schedule the computation on.
   * @param wait a set of events to wait for before starting the
   * computation.
   *
   * @returns a event that when satisfied indicates the computation
   * has completed.
   * @throws std::exception if there is an error scheduling the
   * computation.
   */
  boost::compute::event enqueue(
      out_timeseries_type& out, in_timeseries_type const& in,
      boost::compute::command_queue& queue,
      boost::compute::wait_list const& wait = boost::compute::wait_list()) {
    boost::compute::event event;
    cl_int err = clfftEnqueueTransform(
        p_, d_, 1, &queue.get(), wait.size(), wait.get_event_ptr(),
        &event.get(), &in.get_buffer().get(), &out.get_buffer().get(), nullptr);
    jb::clfft::check_error_code(err, "clfftEnqueueTransform");
    return event;
  }

private:
  // forward declare a helper type to check compile-time constraints.
  struct check_constraints;

  // grant access to create_*_impl functions
  template <typename itype, typename otype>
  friend plan<itype, otype> create_forward_plan_1d(
      otype const&, itype const&, boost::compute::context&,
      boost::compute::command_queue&, int);
  template <typename itype, typename otype>
  friend plan<itype, otype> create_inverse_plan_1d(
      otype const&, itype const&, boost::compute::context&,
      boost::compute::command_queue&, int);

  /**
   * Refactor code to create plans.
   *
   * @param out the output destination prototype.  The actual output
   * in execute() must have the same size as this parameter.
   * @param in the input data prototype.  The actual input in
   * execute() must have the same size as this parameter.
   * @param context a collection of devices in a single OpenCL platform.
   * @param queue a command queue associated with @a context.
   * @param direction the direction of the transform
   * @param batch_size the size of the batch, the size of @a out and
   * @a in must be a multiple of this number.
   *
   * @returns a new plan that computes a DFT (or the inverse DFT) from
   * a vector like @a in into a vector like @a out.
   * @throws std::exception if the parameters are invalid or there was
   * an OpenCL or clFFT error.
   */
  static plan create_plan_1d_impl(
      out_timeseries_type const& out, in_timeseries_type const& in,
      boost::compute::context& context, boost::compute::command_queue& queue,
      clfftDirection direction, std::size_t batch_size) {
    if (out.size() != in.size()) {
      throw std::invalid_argument("clfft::plan - size mismatch");
    }
    if (batch_size == 0) {
      throw std::invalid_argument("clfft::plan - 0 is not a valid batch size");
    }
    if (in.size() % batch_size != 0) {
      throw std::invalid_argument(
          "clfft::plan - the input / output sizes must"
          " be multiples of the batch size");
    }
    // The number of dimension (or rank) of the FFT, all our FFTs are
    // in one dimension, hence the function names ...
    clfftDim dim = CLFFT_1D;

    // ... the length on each dimension, if we had more dimensions
    // this would be an array with 2 or 3 elements ...
    std::size_t lengths[] = {in.size()};

    // ... create a default plan ...
    clfftPlanHandle plan;
    cl_int err = clfftCreateDefaultPlan(&plan, context.get(), dim, lengths);

    // ... clFFT uses ugly error checking, turn into exceptions if
    // needed ...
    jb::clfft::check_error_code(err, "clfftCreateDefaultPlan");

    // ... set the precision ...
    err = clfftSetPlanPrecision(plan, in_value_traits::precision);
    jb::clfft::check_error_code(err, "clfftSetPlanPrecision");

    // ... use the traits to determine the layout of the data ...
    err =
        clfftSetLayout(plan, in_value_traits::layout, out_value_traits::layout);
    jb::clfft::check_error_code(err, "clfftSetLayout");

    // ... store the results of the FFT in a second buffer, it is
    // possible to save memory by storing in the same buffer ...
    err = clfftSetResultLocation(plan, CLFFT_OUTOFPLACE);
    jb::clfft::check_error_code(err, "clfftSetResultLocation");

    // ... the size of the 'batch' ...
    err = clfftSetPlanBatchSize(plan, batch_size);
    jb::clfft::check_error_code(err, "clfftSetPlanBatchSize");

    // ... compute the plan, notice that this blocks because the plan
    // won't be ready until the operations queued here complete ...
    err = clfftBakePlan(plan, 1, &queue.get(), nullptr, nullptr);
    jb::clfft::check_error_code(err, "clfftBakePlan");

    // ... here is the blocking operation ...
    queue.finish();

    // ... return the wrapper ...
    return jb::clfft::plan<in_timeseries_type, out_timeseries_type>(
        plan, direction);
  }

  //@{
  /**
   * @name Deleted functions
   */
  plan(plan const&) = delete;
  plan& operator=(plan const&) = delete;
  //@}

private:
  /// Constructor from a clfftPlanHandle
  explicit plan(clfftPlanHandle p, clfftDirection d)
      : p_(p)
      , d_(d) {
  }

private:
  clfftPlanHandle p_;
  clfftDirection d_;
};

/**
 * Create a forward DFT plan.
 *
 * @param out the output destination prototype.  The actual output
 * in execute() must have the same size as this parameter.
 * @param in the input data prototype.  The actual input in
 * execute() must have the same size as this parameter.
 * @param context a collection of devices in a single OpenCL platform.
 * @param queue a command queue associated with @a context to create
 * and bake the plan.
 *
 * @returns a new plan that computes a DFT from a vector like @a in
 * into a vector like @a out.
 * @throws std::exception if the parameters are invalid or there was
 * an OpenCL or clFFT error.
 * @param batch_size the number of timeseries in the vector.
 *
 * @tparam invector the type of the input vector
 * @tparam outvector the type of the output vector
 */
template <typename invector, typename outvector>
plan<invector, outvector> create_forward_plan_1d(
    outvector const& out, invector const& in, boost::compute::context& ct,
    boost::compute::command_queue& q, int batch_size = 1) {
  return plan<invector, outvector>::create_plan_1d_impl(
      out, in, ct, q, CLFFT_FORWARD, batch_size);
}

/**
 * Create an inverse DFT plan.
 *
 * @param out the output destination prototype.  The actual output
 * in execute() must have the same size as this parameter.
 * @param in the input data prototype.  The actual input in
 * execute() must have the same size as this parameter.
 * @param context a collection of devices in a single OpenCL platform.
 * @param queue a command queue associated with @a context to create
 * and bake the plan.
 * @param batch_size the number of timeseries in the vector.
 *
 * @returns a new plan that computes a DFT from a vector like @a in
 * into a vector like @a out.
 * @throws std::exception if the parameters are invalid or there was
 * an OpenCL or clFFT error.
 *
 * @tparam invector the type of the input vector
 * @tparam outvector the type of the output vector
 */
template <typename invector, typename outvector>
plan<invector, outvector> create_inverse_plan_1d(
    outvector const& out, invector const& in, boost::compute::context& ct,
    boost::compute::command_queue& q, int batch_size = 1) {
  return plan<invector, outvector>::create_plan_1d_impl(
      out, in, ct, q, CLFFT_BACKWARD, batch_size);
}

/**
 * Check the compile-time constraints for a jb::fftw::plan<>
 */
template <typename in_timeseries_type, typename out_timeseries_type>
struct plan<in_timeseries_type, out_timeseries_type>::check_constraints {
  check_constraints() {
    using in_value_type = typename in_timeseries_type::value_type;
    using out_value_type = typename out_timeseries_type::value_type;
    using in_precision_type =
        typename jb::extract_value_type<in_value_type>::precision;
    using out_precision_type =
        typename jb::extract_value_type<out_value_type>::precision;
    static_assert(
        std::is_same<in_precision_type, out_precision_type>::value,
        "Mismatched precision_type, both timeseries must have the same"
        " precision");
    static_assert(
        std::is_same<in_precision_type, float>::value or
            std::is_same<in_precision_type, double>::value,
        "Unsupported precision type, clFFT only supports double or float");
  }
};

} // namespace clfft
} // namespace jb

#endif // jb_plan_hpp
