#include <jb/clfft/plan.hpp>
#include <jb/opencl/device_selector.hpp>
#include <jb/testing/microbenchmark.hpp>

#include <boost/compute/command_queue.hpp>
#include <boost/utility/addressof.hpp>
#include <chrono>
#include <iostream>
#include <string>
#include <stdexcept>

namespace {

int nsamples = 2048;

class fixture {
 public:
  fixture(
      boost::compute::context& context,
      boost::compute::command_queue& q)
      : fixture(nsamples, context, q)
  {}
  fixture(
      int size,
      boost::compute::context& context,
      boost::compute::command_queue& q)
      : src(size)
      , in(size, context)
      , out(size, context)
      , dst(size)
      , queue(q)
      , fft(jb::clfft::create_forward_plan_1d(out, in, context, queue))
  {}

  void run() {
    using boost::compute::wait_list;
    auto upload_done = boost::compute::copy_async(
        src.begin(), src.end(), in.begin(), queue);
    auto fft_done = fft.enqueue(
        out, in, queue, wait_list(upload_done.get_event()));
    // TODO() need to contribute a copy_async() implementation with a
    // wait_list ...
    typedef std::iterator_traits<outvector::iterator>::value_type value_type;
    std::size_t count = boost::compute::detail::iterator_range_size(
        out.begin(), out.end());
    if (count == 0) {
      fft_done.wait();
      return;
    }
    auto first = out.begin();
    boost::compute::buffer const& buffer = first.get_buffer();
    std::size_t offset = out.begin().get_index();
    auto download_done =
        queue.enqueue_read_buffer_async(
            buffer, offset * sizeof(value_type), count * sizeof(value_type),
            boost::addressof(*dst.begin()),
            wait_list(fft_done));
    download_done.wait();
  }

 private:
  typedef boost::compute::vector<std::complex<float>> invector;
  typedef boost::compute::vector<std::complex<float>> outvector;
  std::vector<std::complex<float>> src;
  invector in;
  outvector out;
  std::vector<std::complex<float>> dst;
  boost::compute::command_queue queue;
  jb::clfft::plan<invector, outvector> fft;
};

void benchmark_test_case(
    jb::testing::microbenchmark_config const& cfg) {
  jb::clfft::init init;
  boost::compute::device device = jb::opencl::device_selector();
  boost::compute::context context(device);
  boost::compute::command_queue queue(context, device);
  typedef jb::testing::microbenchmark<fixture> benchmark;
  benchmark bm(cfg);

  auto r = bm.run(context, queue);
  typename benchmark::summary s(r);
  std::cerr << cfg.test_case() << " summary " << s << std::endl;
  if (cfg.verbose()) {
    bm.write_results(std::cout, r);
  }
}

} // anonymous namespace

int main(int argc, char* argv[]) try {
  jb::testing::microbenchmark_config cfg;
  cfg.test_case("complex:float:unaligned").process_cmdline(argc, argv);

  if (cfg.test_case() == "complex:float:unaligned") {
    benchmark_test_case(cfg);
  } else {
    std::ostringstream os;
    os << "Unknown test case (" << cfg.test_case() << ")" << std::endl;
    os << " --test-case must be one of"
       << ": complex:float:unaligned"
       << std::endl;
    throw jb::usage(os.str(), 1);
  }

  return 0;
} catch(jb::usage const& ex) {
  std::cerr << "usage: " << ex.what() << std::endl;
  return ex.exit_status();
} catch(std::exception const& ex) {
  std::cerr << "standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch(...) {
  std::cerr << "unknown exception raised" << std::endl;
  return 1;
}

