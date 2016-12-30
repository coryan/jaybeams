#ifndef jb_clfft_error_hpp
#define jb_clfft_error_hpp

#include <boost/compute/exception/opencl_error.hpp>
#include <stdexcept>

namespace jb {
namespace clfft {

/**
 * A run-time clFFT error.
 *
 * This class represents an error return a clFFT function.
 */
class clfft_error : public std::runtime_error {
 public:
  /// Constructor from a clFFT error code and a message
  clfft_error(cl_int error, char const* msg);
  
  /// Destructor
  ~clfft_error() noexcept
  {}

  /// Returns the numeric error code.
  cl_int error_code() const noexcept {
    return error_;
  }

  /// Convert error code to a string
  static std::string to_string(cl_int error);

 private:
  /// Generate a what() string given an error code and message
  std::string to_what(cl_int error, char const* msg);
  
 private:
  cl_int error_;
  std::string what_;
};

/**
 * Check in an OpenCL error code is really an error and raise an
 * exception if so.
 */
inline void check_error_code(cl_int err, char const* msg) {
  if (err == CL_SUCCESS) {
    return;
  }
  throw jb::clfft::clfft_error(err, msg);
}

} // namespace clfft
} // namespace jb

#endif // jb_cl_error_hpp
