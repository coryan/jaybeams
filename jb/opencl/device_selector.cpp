#include "jb/opencl/device_selector.hpp"
#include <jb/opencl/config.hpp>

#include <iostream>

boost::compute::device jb::opencl::device_selector(config const& cfg) {
  using boost::compute::device;
  if (cfg.device_name() == "BESTCPU") {
    return detail::best_device(
        [](device const& d) { return d.type() & device::cpu; }, "CPU");
  }
  if (cfg.device_name() == "BESTGPU") {
    return detail::best_device(
        [](device const& d) { return d.type() & device::gpu; }, "GPU");
  }
  if (cfg.device_name() == "" or cfg.device_name() == "SYSTEM:DEFAULT") {
    return boost::compute::system::default_device();
  }
  return boost::compute::system::find_device(cfg.device_name());
}

boost::compute::device jb::opencl::device_selector() {
  auto device = jb::opencl::device_selector(jb::opencl::config());
  if (device.name().substr(0, 8) == "AMD SUMO") {
    // TODO(#124) - fix this when we have a better workaround ...
    device = device_selector(jb::opencl::config().device_name("BESTGPU"));
  }
  return device;
}
