#include <jb/clfft/plan.hpp>
#include <jb/clfft/error.hpp>

jb::clfft::plan::~plan() {
  if (dir_ != ENDDIRECTION) {
    cl_int err = clfftDestroyPlan(&p_);
    jb::clfft::check_error_code(err, "clfftDestroyPlan");
  }
}

void jb::clfft::plan::enqueue(
    cl::CommandQueue& queue, cl::Buffer& in, cl::Buffer& out,
    std::vector<cl::Event> const& events, cl::Event& ev) {
  cl_int err = clfftEnqueueTransform(
      p_, dir_, 1, &queue(),
      events.size(),
      (events.size() > 0) ? (cl_event*)&events.front() : nullptr,
      &ev(), &in(), &out(), nullptr);
  jb::clfft::check_error_code(err, "clfftEnqueueTransform");
}

jb::clfft::plan jb::clfft::plan::dft_1d_impl(
    std::size_t N, std::size_t size, clfftDirection dir,
    cl::Context& context, cl::CommandQueue& queue) {
  // The number of dimension (or rank) of the FFT, all our FFTs are in
  // one dimension, hence the function names ...
  clfftDim dim = CLFFT_1D;

  // ... the length on each dimension, if we had more dimensions this
  // would be an array with 2 or 3 elements ...
  size_t lengths[] = { size };

  // ... create a default plan ...
  clfftPlanHandle plan;
  cl_int err = clfftCreateDefaultPlan(&plan, context(), dim, lengths);

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
  err = clfftSetPlanBatchSize(plan, N);
  jb::clfft::check_error_code(err, "clfftSetPlanBatchSize");

  // ... compute the plan, notice that this blocks because the plan
  // won't be ready until the operations queued here complete ...
  err = clfftBakePlan(plan, 1, &queue(), nullptr, nullptr);
  jb::clfft::check_error_code(err, "clfftBakePlan");

  // ... here is the blocking operation ...
  queue.finish();

  // ... return the wrapper ...
  return clfft::plan(plan, dir);
}
