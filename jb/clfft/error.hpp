#ifndef jb_clfft_error_hpp
#define jb_clfft_error_hpp

#include <jb/opencl/platform.hpp>

namespace jb {
namespace clfft {

/**
 * Raise an OpenCL error as an exception.
 *
 * In JayBeams all errors are reported using C++ exceptions, this is
 * used to adapt OpenCL error codes to that protocol.
 */
[[noreturn]] void raise_opencl_error(cl_int err, char const* msg);

/**
 * Check in an OpenCL error code is really an error and raise an
 * exception if so.
 */
inline void check_error_code(cl_int err, char const* msg) {
  if (err == CL_SUCCESS) {
    return;
  }
  raise_opencl_error(err, msg);
}

} // namespace clfft
} // namespace jb

#endif // jb_cl_error_hpp
