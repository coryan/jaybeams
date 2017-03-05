#include <jb/clfft/init.hpp>
#include <jb/clfft/plan.hpp>
#include <jb/opencl/device_selector.hpp>
#include <jb/testing/check_close_enough.hpp>
#include <jb/testing/create_square_timeseries.hpp>

#include <boost/test/unit_test.hpp>
#include <complex>
#include <vector>

/**
 * @test Verify that we can create and destroy default initialized plans.
 */
BOOST_AUTO_TEST_CASE(clfft_plan_default) {
  jb::clfft::init init;
  {
    using invector = boost::compute::vector<std::complex<float>>;
    using outvector = boost::compute::vector<std::complex<float>>;
    using tested_type = jb::clfft::plan<invector, outvector>;
    tested_type x;
    tested_type y;
  }
}

/**
 * @test Verify that we use move copy constructor and assignment on plans.
 */
BOOST_AUTO_TEST_CASE(clfft_plan_move) {
  boost::compute::device device = jb::opencl::device_selector();
  boost::compute::context context(device);
  boost::compute::command_queue queue(context, device);
  jb::clfft::init init;

  std::size_t size = 128;
  using invector = boost::compute::vector<std::complex<float>>;
  using outvector = boost::compute::vector<std::complex<float>>;

  std::vector<std::complex<float>> src(size);
  jb::testing::create_square_timeseries(size, src);

  invector in(size, context);
  outvector out(size, context);

  boost::compute::copy(src.begin(), src.end(), in.begin(), queue);

  {
    auto fft = jb::clfft::create_forward_plan_1d(out, in, context, queue);

    auto p(std::move(fft));

    BOOST_CHECK_NO_THROW(p.enqueue(out, in, queue).wait());
    BOOST_CHECK_THROW(fft.enqueue(out, in, queue), std::exception);
  }
  {
    auto fft = jb::clfft::create_forward_plan_1d(out, in, context, queue);

    jb::clfft::plan<invector, outvector> p;
    p = std::move(fft);
    BOOST_CHECK_NO_THROW(p.enqueue(out, in, queue).wait());
    BOOST_CHECK_THROW(fft.enqueue(out, in, queue), std::exception);
  }
  {
    jb::clfft::plan<invector, outvector> p;

    p = jb::clfft::create_forward_plan_1d(out, in, context, queue);
    BOOST_CHECK_NO_THROW(p.enqueue(out, in, queue).wait());
  }
}

/**
 * @test Verify that we can create clfft_plan objects and they work as expected.
 */
BOOST_AUTO_TEST_CASE(clfft_plan_basic) {
  std::size_t const size = 1 << 8;
  // The max precision loss for an FFT is O(sqrt(N)).  With N == 1<<8
  // the sqrt is 1<<4, but we have 2 FFT operations and the error
  // factors compound (we need to multiply them), so 1<<8 epsilons is
  // a good guess to the maximum error:
  int const tol = 1 << 8;

  boost::compute::device device = jb::opencl::device_selector();
  boost::compute::context context(device);
  boost::compute::command_queue queue(context, device);
  jb::clfft::init init;

  using invector = boost::compute::vector<std::complex<float>>;
  using outvector = boost::compute::vector<std::complex<float>>;

  std::vector<std::complex<float>> src(size);
  jb::testing::create_square_timeseries(size, src);

  invector in(size, context);
  outvector tmp(size, context);
  invector out(size, context);

  auto fft = jb::clfft::create_forward_plan_1d(tmp, in, context, queue);
  auto ifft = jb::clfft::create_inverse_plan_1d(out, tmp, context, queue);

  boost::compute::copy(src.begin(), src.end(), in.begin(), queue);

  fft.enqueue(tmp, in, queue).wait();
  ifft.enqueue(out, tmp, queue).wait();

  std::vector<std::complex<float>> dst(size);
  boost::compute::copy(out.begin(), out.end(), dst.begin(), queue);

  bool res = jb::testing::check_collection_close_enough(dst, src, tol);
  BOOST_CHECK_MESSAGE(res, "collections are not within tolerance=" << tol);
}

/**
 * @test Verify that the plan detects errors and reports them.
 */
BOOST_AUTO_TEST_CASE(clfft_plan_error) {
  using invector = boost::compute::vector<std::complex<float>>;
  using outvector = boost::compute::vector<std::complex<float>>;

  std::size_t const size = 1 << 8;
  boost::compute::device device = jb::opencl::device_selector();
  boost::compute::context context(device);
  boost::compute::command_queue queue(context, device);
  jb::clfft::init init;

  invector in(size, context);
  outvector tmp(size - 1, context);

  BOOST_CHECK_THROW(
      jb::clfft::create_forward_plan_1d(tmp, in, context, queue),
      std::invalid_argument);

  outvector out(size, context);
  BOOST_CHECK_THROW(
      jb::clfft::create_forward_plan_1d(out, in, context, queue, 0),
      std::invalid_argument);
  BOOST_CHECK_THROW(
      jb::clfft::create_forward_plan_1d(out, in, context, queue, 3),
      std::invalid_argument);
}
