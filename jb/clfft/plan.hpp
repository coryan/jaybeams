#ifndef jb_clfft_plan_hpp
#define jb_clfft_plan_hpp

#include <jb/clfft/init.hpp>
#include <jb/opencl/platform.hpp>

namespace jb {
namespace clfft {

/**
 * Wrap clfftPlanHandle objects in a class that can automatically
 * destroy them.
 */
class plan {
 public:
  /// Default constructor
  plan()
      : p_()
      , dir_(ENDDIRECTION) {
  }

  /// Basic move constructor
  plan(plan && rhs)
      : p_(rhs.p_)
      , dir_(rhs.dir_) {
    clfftPlanHandle tmp{};
    rhs.p_ = tmp;
    rhs.dir_ = ENDDIRECTION;
  }

  /// Move assigment operator
  plan& operator=(plan&& rhs) {
    plan tmp(std::move(rhs));
    std::swap(p_, tmp.p_);
    std::swap(dir_, tmp.dir_);
    return *this;
  }

  // Destructor, cleanup the plan.
  ~plan();

  /// Queue the transform
  void enqueue(
      cl::CommandQueue& queue, cl::Buffer& in, cl::Buffer& out,
      std::vector<cl::Event> const& wait_events, cl::Event& ev);

  /// Create a plan to perform a direct FFT
  static plan dft_1d(
      std::size_t size, cl::Context& context, cl::CommandQueue& queue) {
    return dft_1d_impl(1, size, CLFFT_FORWARD, context, queue);
  }

  /// Create a plan to perform an inverse FFT
  static plan dft_1d_inv(
      std::size_t size, cl::Context& context, cl::CommandQueue& queue) {
    return dft_1d_impl(1, size, CLFFT_BACKWARD, context, queue);
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
  explicit plan(clfftPlanHandle p, clfftDirection dir)
      : p_(p)
      , dir_(dir) {
  }

  /// Common implementation for all plan creation functions
  static plan dft_1d_impl(
      std::size_t N, std::size_t size, clfftDirection dir,
      cl::Context& context, cl::CommandQueue& queue);

 private:
  clfftPlanHandle p_;
  clfftDirection dir_;
};

} // namespace clfft
} // namespace jb

#endif // jb_plan_hpp
