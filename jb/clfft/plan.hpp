#ifndef jb_clfft_plan_hpp
#define jb_clfft_plan_hpp

#include <jb/clfft/error.hpp>
#include <jb/clfft/init.hpp>
#include <jb/opencl/platform.hpp>

#include <boost/compute/command_queue.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/event.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/utility/wait_list.hpp>

namespace jb {
namespace clfft {

/**
 * Wrap clfftPlanHandle objects in a class that can automatically
 * destroy them.
 */
template<typename invector, typename outvector>
class plan {
 public:
  /// Default constructor
  plan() : p_(), d_(ENDDIRECTION) {}

  /// Basic move constructor
  plan(plan&& rhs)
      : p_(rhs.p_), d_(rhs.d_) {
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

  // Destructor, cleanup the plan.
  ~plan() {
    if (d_ == ENDDIRECTION) {
      return;
    }
    cl_int err = clfftDestroyPlan(&p_);
    jb::clfft::check_error_code(err, "clfftDestroyPlan");
  }

  /// Queue the transform
  boost::compute::event enqueue(
      outvector& out, invector const& in,
      boost::compute::command_queue& queue,
      boost::compute::wait_list const& wait = boost::compute::wait_list()) {
    boost::compute::event event;
    cl_int err = clfftEnqueueTransform(
        p_, d_, 1, &queue.get(),
        wait.size(), wait.get_event_ptr(), &event.get(),
        &in.get_buffer().get(), &out.get_buffer().get(), nullptr);
    jb::clfft::check_error_code(err, "clfftEnqueueTransform");
    return event;
  }

  /// Create a forward FFT for the given types and sizes.
  static plan create_forward_plan_1d(
      outvector const& out, invector const& in,
      boost::compute::context& context, boost::compute::command_queue& queue) {
    return create_plan_1d_impl(out, in, context, queue, CLFFT_FORWARD);
  }

  /// Create an inverse FFT for the given types and sizes.
  static plan create_inverse_plan_1d(
      outvector const& out, invector const& in,
      boost::compute::context& context, boost::compute::command_queue& queue) {
    return create_plan_1d_impl(out, in, context, queue, CLFFT_BACKWARD);
  }

 private:
  /// Refactor common code to create forward and backward plan.
  static plan create_plan_1d_impl(
      outvector const& out, invector const& in,
      boost::compute::context& context,
      boost::compute::command_queue& queue,
      clfftDirection direction) {
    if (out.size() != in.size()) {
      throw std::invalid_argument("clfft::plan - size mismatch");
    }
    // The number of dimension (or rank) of the FFT, all our FFTs are
    // in one dimension, hence the function names ...
    clfftDim dim = CLFFT_1D;

    // ... the length on each dimension, if we had more dimensions
    // this would be an array with 2 or 3 elements ...
    std::size_t lengths[] = { in.size() };

    // ... create a default plan ...
    clfftPlanHandle plan;
    cl_int err = clfftCreateDefaultPlan(&plan, context.get(), dim, lengths);

    // ... clFFT uses ugly error checking, turn into exceptions if
    // needed ...
    jb::clfft::check_error_code(err, "clfftCreateDefaultPlan");

    // ... set the precision, our plans are always in 'float' ...
    err = clfftSetPlanPrecision(plan, CLFFT_SINGLE);
    jb::clfft::check_error_code(err, "clfftSetPlanPrecision");

    // ... our data is always RIRIRI (i.e. real-part, imaginary-part) ...
    err = clfftSetLayout(
        plan, CLFFT_COMPLEX_INTERLEAVED, CLFFT_COMPLEX_INTERLEAVED);
    jb::clfft::check_error_code(err, "clfftSetLayout");

    // ... store the results of the FFT in a second buffer, it is
    // possible to save memory by storing in the same buffer ...
    err = clfftSetResultLocation(plan, CLFFT_OUTOFPLACE);
    jb::clfft::check_error_code(err, "clfftSetResultLocation");

    // ... the size of the 'batch' ...
    err = clfftSetPlanBatchSize(plan, 1);
    jb::clfft::check_error_code(err, "clfftSetPlanBatchSize");

    // ... compute the plan, notice that this blocks because the plan
    // won't be ready until the operations queued here complete ...
    err = clfftBakePlan(plan, 1, &queue.get(), nullptr, nullptr);
    jb::clfft::check_error_code(err, "clfftBakePlan");

    // ... here is the blocking operation ...
    queue.finish();

    // ... return the wrapper ...
    return jb::clfft::plan<invector, outvector>(plan, direction); 
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
      : p_(p), d_(d) {}

 private:
  clfftPlanHandle p_;
  clfftDirection d_;
};

template<typename invector, typename outvector>
plan<invector, outvector> create_forward_plan_1d(
    outvector const& out, invector const& in,
    boost::compute::context& ct, boost::compute::command_queue& q) {
  return plan<invector, outvector>::create_forward_plan_1d(out, in, ct, q);
}

template<typename invector, typename outvector>
plan<invector, outvector> create_inverse_plan_1d(
    outvector const& out, invector const& in,
    boost::compute::context& ct, boost::compute::command_queue& q) {
  return plan<invector, outvector>::create_inverse_plan_1d(out, in, ct, q);
}


} // namespace clfft
} // namespace jb

#endif // jb_plan_hpp
