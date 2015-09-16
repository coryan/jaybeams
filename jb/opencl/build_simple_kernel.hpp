#ifndef jb_opencl_build_simple_kernel_hpp
#define jb_opencl_build_simple_kernel_hpp

#include <jb/opencl/platform.hpp>
#include <iosfwd>

namespace jb {
namespace opencl {

/**
 * Build a simple program (one where everything is in a single string)
 * and get a kernel from it.
 *
 * Most of our kernels are relatively simple, so this routine is very
 * convenient.
 */
cl::Kernel build_simple_kernel(
    cl::Context context, cl::Device device,
    char const* code, char const* kernel_name);

/**
 * Build a simple program from an iostream and get a kernel from the result.
 */
cl::Kernel build_simple_kernel(
    cl::Context context, cl::Device device,
    std::istream& code, char const* kernel_name);

/**
 * Convenience function to build a simple program and return it.
 *
 * Build a problem that consists of a single source string, targeted
 * to a single device.
 */
cl::Program build_simple_program(
    cl::Context context, cl::Device device, char const* code);

/**
 * Convenience function to build a simple program and return it.
 */
cl::Program build_simple_program(
    cl::Context context, cl::Device device, std::istream& code);

} // namespace opencl
} // namespace jb

#endif // jb_opencl_build_simple_kernel_hpp
