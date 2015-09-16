#include <jb/opencl/build_simple_kernel.hpp>
#include <jb/log.hpp>

#include <fstream>

cl::Kernel jb::opencl::build_simple_kernel(
    cl::Context context, cl::Device device,
    char const* code, char const* kernel_name) {
  auto program = build_simple_program(context, device, code);
  return cl::Kernel(program, kernel_name);
}

cl::Kernel jb::opencl::build_simple_kernel(
    cl::Context context, cl::Device device,
    std::istream& code, char const* kernel_name) {
  auto program = build_simple_program(context, device, code);
  return cl::Kernel(program, kernel_name);
}

cl::Program jb::opencl::build_simple_program(
    cl::Context context, cl::Device device, char const* code) {
  // ... create the program, a program may consist of multiple sources
  // and have many kernels in it, but in our case it is rather simple ...
  auto p = std::make_pair(code, std::strlen(code) - 1);
  cl::Program::Sources sources({p});

  cl::Program program(context, sources);
  try {
    program.build(std::vector<cl::Device>{device});
  } catch(cl::Error const& ex) {
    std::string results;
    program.getBuildInfo(device, CL_PROGRAM_BUILD_LOG, &results);
    JB_LOG(error)<< "errors building program:\n"
                 << results
                 << "\n";
    throw;
  }

  return program;
}

cl::Program jb::opencl::build_simple_program(
    cl::Context context, cl::Device device, std::istream& code) {
  std::string tmp;
  std::string line;
  while (std::getline(code, line)) {
    tmp += line;
    tmp += "\n";
  }
  return build_simple_program(context, device, tmp.c_str());
}
