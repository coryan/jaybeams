#include <jb/opencl/device_selector.hpp>
#include <jb/opencl/config.hpp>

#include <iostream>

cl::Device jb::opencl::device_selector(config const& cfg) {
  std::vector<cl::Platform> platforms;
  (void) cl::Platform::get(&platforms);
  if (platforms.empty()) {
    throw std::runtime_error("Cannot find any OpenCL platform");
  }

  for (auto const& p : platforms) {
    std::vector<cl::Device> devices;
    p.getDevices(CL_DEVICE_TYPE_ALL, &devices);

    for (auto const& d : devices) {
      if (d.getInfo<CL_DEVICE_NAME>() == cfg.device_name()) {
        return d;
      }
    }
  }

  // Not searching by name or name not found, find the GPU with the
  // largest number of compute units ...
  cl::Device best_gpu;
  cl_uint best_gpu_count = 0;
  cl::Device best_cpu;
  cl_uint best_cpu_count = 0;
  for (auto const& p : platforms) {
    std::vector<cl::Device> devices;
    p.getDevices(CL_DEVICE_TYPE_ALL, &devices);

    for (auto const& d : devices) {
      if (d.getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_GPU) {
        if (d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() > best_gpu_count) {
          best_gpu_count = d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
          best_gpu = d;
        }
        continue;
      }
      if (d.getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU) {
        if (d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() > best_cpu_count) {
          best_cpu_count = d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
          best_cpu = d;
        }
        continue;
      }
    }
  }

  if (best_gpu_count != 0) {
    return best_gpu;
  }
    
  return best_cpu;
}

cl::Device jb::opencl::device_selector() {
  return jb::opencl::device_selector(jb::opencl::config());
}
