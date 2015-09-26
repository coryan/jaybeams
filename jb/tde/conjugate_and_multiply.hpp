#ifndef jb_tde_conjugate_and_multiply_hpp
#define jb_tde_conjugate_and_multiply_hpp

#include <jb/tde/conjugate_and_multiply_kernel.hpp>
#include <jb/complex_traits.hpp>
#include <boost/compute/algorithm/copy.hpp>
#include <boost/compute/container/vector.hpp>
#include <boost/compute/buffer.hpp>

namespace jb {
namespace tde {

template<typename T>
struct conjugate_and_multiply_traits;

template<>
struct conjugate_and_multiply_traits<float> {
  static std::string flags() {
    return std::string("-DTYPENAME_MACRO=float2");
  }
  static std::string program_name() {
    return std::string("__jaybeams_conjugate_and_multiply_float");
  }
};

template<>
struct conjugate_and_multiply_traits<double> {
  static std::string flags() {
    return std::string("-DTYPENAME_MACRO=double2");
  }
  static std::string program_name() {
    return std::string("__jaybeams_conjugate_and_multiply_double");
  }
};

template<typename precision_t>
boost::compute::kernel conjugate_and_multiply_kernel(
    boost::compute::context context) {
  typedef conjugate_and_multiply_traits<precision_t> traits;
  auto cache = boost::compute::program_cache::get_global_cache(context);
  auto program = cache->get_or_build(
      traits::program_name(), traits::flags(),
      conjugate_and_multiply_kernel_source, context);
  return program.create_kernel("conjugate_and_multiply");
}

template<typename InputIterator, typename OutputIterator>
boost::compute::future<OutputIterator>
conjugate_and_multiply(
    InputIterator a_start, InputIterator a_end,
    InputIterator b_start, InputIterator b_end,
    OutputIterator output, boost::compute::command_queue& queue,
    boost::compute::wait_list const& wait = boost::compute::wait_list()) {
  typedef typename std::iterator_traits<InputIterator>::value_type
      input_value_type;
  typedef typename std::iterator_traits<OutputIterator>::value_type
      output_value_type;
  typedef typename jb::extract_value_type<input_value_type>::precision
      precision_type;

  namespace bc = boost::compute;
  namespace bcdetail = boost::compute::detail;

  static_assert(
      std::is_same<input_value_type, output_value_type>::value,
      "jb::td::conjugate_and_multiply() input and output value types"
      " must be identical");
  static_assert(
      std::is_same<std::complex<precision_type>, output_value_type>::value,
      "jb::td::conjugate_and_multiply() value type must be an instance"
      " of std::complex");
  static_assert(
      bcdetail::is_device_iterator<InputIterator>::value,
      "jb::td::conjugate_and_multiply() input range must be"
      " a device container");
  static_assert(
      bcdetail::is_device_iterator<OutputIterator>::value,
      "jb::td::conjugate_and_multiply() output range must be"
      " a device container");

  std::size_t a_count = bcdetail::iterator_range_size(a_start, a_end);
  if (a_count == 0) {
    return bc::future<OutputIterator>();
  }
  std::size_t b_count = bcdetail::iterator_range_size(b_start, b_end);
  if (b_count != a_count) {
    throw std::invalid_argument(
        "jb::td::conjugate_and_multiply() mismatched range sizes");
  }

  bc::buffer const& a_buffer = a_start.get_buffer();
  std::size_t a_offset = a_start.get_index();
  bc::buffer const& b_buffer = b_start.get_buffer();
  bc::buffer const& dst_buffer = output.get_buffer();

  auto kernel = conjugate_and_multiply_kernel<precision_type>(
      queue.get_context());
  kernel.set_arg(0, dst_buffer);
  kernel.set_arg(1, a_buffer);
  kernel.set_arg(2, b_buffer);
  kernel.set_arg(3, cl_uint(a_count));

  auto event =
      queue.enqueue_1d_range_kernel(kernel, a_offset, a_count, 0, wait);

  return bc::make_future(
      bcdetail::iterator_plus_distance(output, a_count), event);
}

} // namespace tde
} // namespace jb

#endif // jb_tde_conjugate_and_multiply_hpp
