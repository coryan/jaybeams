#include <jb/opencl/config.hpp>

namespace jb {
namespace opencl {
namespace defaults {

#ifndef JB_DEFAULT_device_name
#define JB_DEFAULT_device_name ""
#endif // JB_DEFAULT_device_name

std::string device_name(JB_DEFAULT_device_name);

} // namespace defaults
} // namespace opencl
} // namespace jb

jb::opencl::config::config()
    : device_name(
          desc("device-name")
              .help(
                  "When selecting an OpenCL device, prefer those matching this "
                  "name.  If the name is empty (or no device by that name is "
                  "matched) select the GPU device with the largest number of "
                  "compute units. "
                  "If no GPU device is available, select a CPU device."),
          this, defaults::device_name) {
}
