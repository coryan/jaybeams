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

template<bool pipelined>
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
    boost::compute::copy(
        src.begin(), src.end(), in.begin(), queue);
    fft.enqueue(out, in, queue).wait();
    boost::compute::copy(
        out.begin(), out.end(), dst.begin(), queue);
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

template<>
void fixture<true>::run() {
  using namespace boost::compute;
  auto upload_done = copy_async(
      src.begin(), src.end(), in.begin(), queue);
  auto fft_done = fft.enqueue(
      out, in, queue, wait_list(upload_done.get_event()));
  // TODO() need to contribute a copy_async() implementation with a
  // wait_list ...
  auto first = out.begin();
  auto last = out.end();
  typedef std::iterator_traits<outvector::iterator>::value_type value_type;
  std::size_t count = detail::iterator_range_size(first, last);
  if (count == 0) {
    fft_done.wait();
    return;
  }
  buffer const& buffer = first.get_buffer();
  std::size_t offset = first.get_index();
  auto download_done =
      queue.enqueue_read_buffer_async(
          buffer, offset * sizeof(value_type), count * sizeof(value_type),
          ::boost::addressof(*dst.begin()),
          wait_list(fft_done));
  download_done.wait();
}

template<bool pipelined>
void benchmark_test_case(
    jb::testing::microbenchmark_config const& cfg) {
  jb::clfft::init init;
  boost::compute::device device = jb::opencl::device_selector();
  boost::compute::context context(device);
  boost::compute::command_queue queue(context, device);
  typedef jb::testing::microbenchmark<fixture<pipelined>> benchmark;
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
  cfg.test_case("complex:float:async").process_cmdline(argc, argv);

  std::cout << "Configuration for test\n" << cfg << std::endl;

  if (cfg.test_case() == "complex:float:async") {
    benchmark_test_case<true>(cfg);
  } else if (cfg.test_case() == "complex:float:sync") {
    benchmark_test_case<false>(cfg);
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

