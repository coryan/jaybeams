#include "jb/opencl/device_selector.hpp"
#include <jb/opencl/config.hpp>

#include <iostream>

namespace {
boost::compute::device device_selector(std::string const& name) {
  using boost::compute::device;
  if (name == "BESTCPU") {
    return jb::opencl::detail::best_device(
        [](device const& d) { return d.type() & device::cpu; }, "CPU");
  }
  if (name == "BESTGPU") {
    return jb::opencl::detail::best_device(
        [](device const& d) { return d.type() & device::gpu; }, "GPU");
  }
  if (name == "" or name == "SYSTEM:DEFAULT") {
    return boost::compute::system::default_device();
  }
  return boost::compute::system::find_device(name);
}
} // anonymous namespace

boost::compute::device jb::opencl::device_selector(config const& cfg) {
  auto device = ::device_selector(cfg.device_name());
  if (device.name().substr(0, 8) == "AMD SUMO" and cfg.device_name() == "") {
    // LCOV_EXCL_START
    // TODO(#124) - fix this when we have a better workaround ...
    device = device_selector(jb::opencl::config().device_name("BESTGPU"));
    // LCOV_EXCL_STOP
  }
  return device;
}

boost::compute::device jb::opencl::device_selector() {
  return jb::opencl::device_selector(jb::opencl::config());
}
