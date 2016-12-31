#ifndef jb_opencl_copy_to_host_async_hpp
#define jb_opencl_copy_to_host_async_hpp

#include <boost/compute/algorithm/copy.hpp>

namespace jb {
namespace opencl {

/**
 * Copy a device buffer to the host asynchronously.
 *
 * Copies the range [@a first, @a last) from the device to the host,
 * storing the results at @a result.  The copy is started after all
 * the events in @a wait have completed.  The function returns a
 * future object so you can wait for the copy to complete, and chain
 * other operations.
 *
 * @tparam DeviceIterator the iterator type for the container on the device
 * @tparam HostIterator the iterator type for the container on the
 * host
 * @param first the beginning of the range to copy
 * @param last the end of the range to copy
 * @param result the beginning of the range to store the copy
 * @param queue command queue to perform the operation
 * @param wait a list of events (potentially empty) to wait for before
 * starting the copy
 * @returns A Future, once fulfilled it points to the end of the
 * result range.
 */
template <class DeviceIterator, class HostIterator>
inline boost::compute::future<HostIterator> copy_to_host_async(
    DeviceIterator first, DeviceIterator last, HostIterator result,
    boost::compute::command_queue& queue,
    boost::compute::wait_list const& wait = boost::compute::wait_list()) {
  namespace bc = boost::compute;
  namespace bcdetail = boost::compute::detail;

  static_assert(
      bc::is_device_iterator<DeviceIterator>::value,
      "jb::opencl::copy_to_host_async() input range must be"
      " a device container");
  static_assert(
      not bc::is_device_iterator<HostIterator>::value,
      "jb::opencl::copy_to_host_async() input range must be"
      " a host container");
  static_assert(
      bcdetail::is_contiguous_iterator<HostIterator>::value,
      "jb::opencl::copy_to_host_async() is not supported"
      " for non-contiguous iterators");

  typedef typename std::iterator_traits<DeviceIterator>::value_type value_type;

  std::size_t count = bcdetail::iterator_range_size(first, last);
  bc::buffer const& buffer = first.get_buffer();
  std::size_t offset = first.get_index();

  auto event = queue.enqueue_read_buffer_async(
      buffer, offset * sizeof(value_type), count * sizeof(value_type),
      ::boost::addressof(*result), wait);

  return bc::make_future(
      bcdetail::iterator_plus_distance(result, count), event);
}

} // namespace opencl
} // namespace jb

#endif // jb_opencl_copy_to_host_async_hpp
