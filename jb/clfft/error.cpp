#include <jb/clfft/error.hpp>

#include <sstream>
#include <stdexcept>

void jb::clfft::raise_opencl_error(cl_int err, char const* msg) {
  std::ostringstream os;
  os << "OpenCL error " << msg << ": err=" << err;
  throw std::runtime_error(os.str());
}
