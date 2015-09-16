#ifndef jb_opencl_platform_hpp
#define jb_opencl_platform_hpp
/**
 * @file
 *
 * Hide the platform specific details about OpenCL header naming.
 */

#include <jb/jb-config.hpp>

#if defined(HAVE_OPENCL_APPLE_HEADER)
# include <OpenCL/cl.hpp>
#else
# include <CL/cl.hpp>
#endif // HAVE_OPENCL_APPLE_HEADER

namespace jb {
namespace opencl {

/// Define the preferred type to handling lists of cl::Events
typedef std::vector<cl::Event> event_set;

/// Create a cl::Context from a device (work around CL 1.1 limitations)
inline cl::Context device2context(cl::Device const& device) {
  return cl::Context(std::vector<cl::Device>({device}));
}

} // namespace opencl
} // namespace jb

#endif // jb_opencl_platform_hpp
