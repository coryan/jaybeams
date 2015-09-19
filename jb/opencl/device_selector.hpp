#ifndef jb_opencl_device_selector_hpp
#define jb_opencl_device_selector_hpp

#include <boost/compute/device.hpp>

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

} // namespace opencl
} // namespace jb

#endif // jb_cl_device_selector_hpp
