#ifndef jb_opencl_device_selector_hpp
#define jb_opencl_device_selector_hpp

#include <jb/opencl/platform.hpp>

namespace jb {
namespace opencl {

class config;

/**
 * Select an OpenCL device matching the current configuration.
 */
cl::Device device_selector(config const& cfg);

/**
 * Return the default OpenCL device.
 */
cl::Device device_selector();

} // namespace opencl
} // namespace jb

#endif // jb_cl_device_selector_hpp
