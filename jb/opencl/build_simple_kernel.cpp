#include <jb/opencl/build_simple_kernel.hpp>
#include <jb/log.hpp>

#include <fstream>

boost::compute::kernel jb::opencl::build_simple_kernel(
    boost::compute::context context, boost::compute::device device,
    char const* code, char const* kernel_name) {
  auto program = build_simple_program(context, device, code);
  return boost::compute::kernel(program, kernel_name);
}

boost::compute::kernel jb::opencl::build_simple_kernel(
    boost::compute::context context, boost::compute::device device,
    std::istream& code, char const* kernel_name) {
  auto program = build_simple_program(context, device, code);
  return boost::compute::kernel(program, kernel_name);
}

boost::compute::program jb::opencl::build_simple_program(
    boost::compute::context context, boost::compute::device device,
    char const* code) {
  // ... create the program, a program may consist of multiple sources
  // and have many kernels in it, but in our case it is rather simple ...
  boost::compute::program program =
      boost::compute::program::create_with_source(code, context);
  try {
    program.build();
  } catch (boost::compute::opencl_error const& ex) {
    std::string results;
    JB_LOG(error) << "errors building program: " << ex.what() << "\n"
                  << program.build_log() << "\n";
    throw;
  }
  return program;
}

boost::compute::program jb::opencl::build_simple_program(
    boost::compute::context context, boost::compute::device device,
    std::istream& code) {
  std::string tmp;
  std::string line;
  while (std::getline(code, line)) {
    tmp += line;
    tmp += "\n";
  }
  return build_simple_program(context, device, tmp.c_str());
}
