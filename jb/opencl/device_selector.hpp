#ifndef jb_opencl_device_selector_hpp
#define jb_opencl_device_selector_hpp

#include <boost/compute/device.hpp>
#include <boost/compute/system.hpp>

namespace jb {
namespace opencl {

class config;

/**
 * Select an OpenCL device matching the current configuration.
 */
boost::compute::device device_selector(config const& cfg);

/**
 * Return the default OpenCL device.
 */
boost::compute::device device_selector();

namespace detail {

/// Return the best available device of the given type
template <typename Filter>
boost::compute::device best_device(Filter filter, char const* filter_name) {
  boost::compute::device selection;
  unsigned int count = 0;
  for (auto const& d : boost::compute::system::devices()) {
    if (filter(d) and d.compute_units() > count) {
      count = d.compute_units();
      selection = d;
    }
  }
  if (count == 0) {
    std::ostringstream os;
    os << "Could not find a device using filter " << filter_name;
    throw std::runtime_error(os.str());
  }
  return selection;
}

} // namespace detail

} // namespace opencl
} // namespace jb

#endif // jb_cl_device_selector_hpp
