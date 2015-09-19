#include <jb/opencl/device_selector.hpp>
#include <jb/opencl/config.hpp>

#include <boost/compute/system.hpp>
#include <iostream>

namespace {

/// Return the best available device of the given type
boost::compute::device best_device(int device_type, char const *type_name) {
  boost::compute::device selection;
  unsigned int count = 0;
  for (auto const& d : boost::compute::system::devices()) {
    if (d.type() == device_type and d.compute_units() > count) {
      count = d.compute_units();
      selection = d;
    }
  }
  if (count == 0) {
    std::ostringstream os;
    os << "Could not find a device of type " << type_name
       << " (" << device_type << ")";
    throw std::runtime_error(os.str());
  }
  return selection;
}

} // anonymous namespace

boost::compute::device jb::opencl::device_selector(config const& cfg) {
  if (cfg.device_name() == "BESTCPU") {
    return best_device(boost::compute::device::cpu, "CPU");
  }
  if (cfg.device_name() == "BESTGPU") {
    return best_device(boost::compute::device::gpu, "GPU");
  }
  if (cfg.device_name() == "") {
    return boost::compute::system::default_device();
  }
  return boost::compute::system::find_device(cfg.device_name());
}

boost::compute::device jb::opencl::device_selector() {
  return jb::opencl::device_selector(jb::opencl::config());
}
