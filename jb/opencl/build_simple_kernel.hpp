#ifndef jb_opencl_build_simple_kernel_hpp
#define jb_opencl_build_simple_kernel_hpp

#include <boost/compute/context.hpp>
#include <boost/compute/device.hpp>
#include <boost/compute/kernel.hpp>
#include <boost/compute/program.hpp>
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
boost::compute::kernel build_simple_kernel(
    boost::compute::context context, boost::compute::device device,
    char const* code, char const* kernel_name);

/**
 * Build a simple program from an iostream and get a kernel from the result.
 */
boost::compute::kernel build_simple_kernel(
    boost::compute::context context, boost::compute::device device,
    std::istream& code, char const* kernel_name);

/**
 * Convenience function to build a simple program and return it.
 *
 * Build a problem that consists of a single source string, targeted
 * to a single device.
 */
boost::compute::program build_simple_program(
    boost::compute::context context, boost::compute::device device,
    char const* code);

/**
 * Convenience function to build a simple program and return it.
 */
boost::compute::program build_simple_program(
    boost::compute::context context, boost::compute::device device,
    std::istream& code);

} // namespace opencl
} // namespace jb

#endif // jb_opencl_build_simple_kernel_hpp
