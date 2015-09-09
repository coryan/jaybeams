#include <jb/opencl/device_selector.hpp>
#include <jb/opencl/config.hpp>

int main(int argc, char* argv[]) try {
  jb::opencl::config cfg;
  cfg.process_cmdline(argc, argv);
  cl::Device dev = jb::opencl::device_selector(cfg);

  std::cout << "jb::opencl::device_selector picked "
            << dev.getInfo<CL_DEVICE_NAME>() << std::endl;
  return 0;
} catch(jb::usage const& ex) {
  std::cerr << "usage: " << ex.what() << std::endl;
  return ex.exit_status();
} catch(std::exception const& ex) {
  std::cerr << "standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch(...) {
  std::cerr << "unknown exception raised" << std::endl;
  return 1;
}

