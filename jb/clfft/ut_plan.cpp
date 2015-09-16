#include <jb/clfft/plan.hpp>
#include <jb/clfft/init.hpp>
#include <jb/opencl/device_selector.hpp>
#include <jb/testing/create_square_timeseries.hpp>
#include <jb/testing/check_vector_close_enough.hpp>

#include <boost/test/unit_test.hpp>
#include <complex>
#include <vector>

typedef std::vector<std::complex<float>> timeseries_type;

/**
 * @test Verify that we can create and destroy default initialized plans.
 */
BOOST_AUTO_TEST_CASE(clfft_plan_default) {
  jb::clfft::init init;
  {
    jb::clfft::plan x;
    jb::clfft::plan y;
    jb::clfft::plan z;
  }
}

/**
 * @test Verify that we use move copy constructor and assignment on plans.
 */
BOOST_AUTO_TEST_CASE(clfft_plan_move) {
  cl::Device device = jb::opencl::device_selector();
  cl::Context context = jb::opencl::device2context(device);
  cl::CommandQueue queue(context, device);
  jb::clfft::init init;

  std::size_t size = 128;
  cl::Buffer in_buf(context, CL_MEM_READ_WRITE, size * 2 * sizeof(cl_float));
  cl::Buffer tmp_buf(context, CL_MEM_READ_WRITE, size * 2 * sizeof(cl_float));
  timeseries_type in(size);

  jb::opencl::event_set upload(1);
  queue.enqueueWriteBuffer(
      in_buf, false, 0, size * 2 * sizeof(cl_float), &in[0],
      nullptr, &upload[0]);


  {
    jb::clfft::plan fft = jb::clfft::plan::dft_1d(size, context, queue);

    jb::clfft::plan p(std::move(fft));

    cl::Event ev;
    BOOST_CHECK_NO_THROW(p.enqueue(queue, in_buf, tmp_buf, upload, ev));
    BOOST_CHECK_THROW(fft.enqueue(queue, in_buf, tmp_buf, upload, ev), std::exception);
  }
  {
    jb::clfft::plan fft = jb::clfft::plan::dft_1d(size, context, queue);

    jb::clfft::plan p;
    p = std::move(fft);
    cl::Event ev;
    BOOST_CHECK_NO_THROW(p.enqueue(queue, in_buf, tmp_buf, upload, ev));
    BOOST_CHECK_THROW(fft.enqueue(queue, in_buf, tmp_buf, upload, ev), std::exception);
  }
  {
    jb::clfft::plan p;

    p = std::move(jb::clfft::plan::dft_1d(size, context, queue));
    cl::Event ev;
    BOOST_CHECK_NO_THROW(p.enqueue(queue, in_buf, tmp_buf, upload, ev));
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

  cl::Device device = jb::opencl::device_selector();
  cl::Context context = jb::opencl::device2context(device);
  cl::CommandQueue queue(context, device);
  jb::clfft::init init;

  timeseries_type in(size);
  timeseries_type out(size);

  jb::testing::create_square_timeseries(size, in);

  jb::clfft::plan fft = jb::clfft::plan::dft_1d(size, context, queue);
  jb::clfft::plan ifft = jb::clfft::plan::dft_1d_inv(size, context, queue);

  cl::Buffer in_buf(context, CL_MEM_READ_WRITE, size * 2 * sizeof(cl_float));
  cl::Buffer tmp_buf(context, CL_MEM_READ_WRITE, size * 2 * sizeof(cl_float));
  cl::Buffer out_buf(context, CL_MEM_READ_WRITE, size * 2 * sizeof(cl_float));

  jb::opencl::event_set upload(1);
  queue.enqueueWriteBuffer(
      in_buf, false, 0, size * 2 * sizeof(cl_float), &in[0],
      nullptr, &upload[0]);

  jb::opencl::event_set fft_done(1);
  fft.enqueue(queue, in_buf, tmp_buf, upload, fft_done[0]);

  jb::opencl::event_set ifft_done(1);
  ifft.enqueue(queue, tmp_buf, out_buf, fft_done, ifft_done[0]);

  cl::Event read_done;
  queue.enqueueReadBuffer(
      out_buf, false, 0, size * 2 * sizeof(cl_float), &out[0],
      &ifft_done, &read_done);

  read_done.wait();

  jb::testing::check_vector_close_enough(in, out, tol);
}
