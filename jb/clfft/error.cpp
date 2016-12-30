#include "jb/clfft/error.hpp"

#include <boost/compute/exception/opencl_error.hpp>
#include <clFFT.h>
#include <sstream>
#include <utility>

jb::clfft::clfft_error::clfft_error(cl_int error, char const* msg)
    : std::runtime_error(to_what(error, msg))
    , error_(error) {
}

std::string jb::clfft::clfft_error::to_string(cl_int error) {
  // This is a bit ugly, we cheat and use implementation details for
  // the clFFT library to "know" that error codes below CLFFT_BUGCHECK
  // are just regular OpenCL error codes.  It is less ugly than
  // reproducting the code from
  // boost::compute::opencl_error::to_string() though.
  if (error < CLFFT_BUGCHECK) {
    return boost::compute::opencl_error::to_string(error);
  }
  //
  switch (error) {
  case CLFFT_BUGCHECK:
    return "bugcheck";
  case CLFFT_NOTIMPLEMENTED:
    return "functionality is not implemented yet";
  case CLFFT_TRANSPOSED_NOTIMPLEMENTED:
    return "transposed functionality is not implemented"
           " for this transformation";
  case CLFFT_FILE_NOT_FOUND:
    return "tried to open an existing file on the host system, but failed";
  case CLFFT_FILE_CREATE_FAILURE:
    return "tried to create a file on the host system, but failed";
  case CLFFT_VERSION_MISMATCH:
    return "version conflict between client and library";
  case CLFFT_INVALID_PLAN:
    return "requested plan could not be found";
  case CLFFT_DEVICE_NO_DOUBLE:
    return "double precision not supported on this device";
  case CLFFT_DEVICE_MISMATCH:
    return "attempt to run on a device using a plan"
           " baked for a different device";
  case CLFFT_ENDSTATUS:
    return "ENDSTATUS - first error code out of range";
  }
  return "unknown error code";
}

std::string jb::clfft::clfft_error::to_what(cl_int error, char const* msg) {
  std::ostringstream os;
  os << msg << ": " << to_string(error) << " (" << error << ")";
  return os.str();
}
