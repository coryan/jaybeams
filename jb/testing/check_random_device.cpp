/**
 * @file Generate some output from std::random_device.
 *
 * I am interested in examining the output from std::random_device to
 * evaluate its statistical strength.
 */
#include <jb/config_object.hpp>

#include <iostream>
#include <random>

/**
 * Define types and functions used in the program.
 */
namespace {
/// Configuration parameters for bm_order_book
class config : public jb::config_object {
public:
  config();
  config_object_constructors(config);
  void validate() const override;
  jb::config_attribute<config, int> iterations;
  jb::config_attribute<config, std::string> token;
};

void produce_output(std::random_device& rd, int iterations) {
  for (int i = 0; i != iterations; ++i) {
    std::cout << rd() << "\n";
  }
}

char const default_initialization_marker[] = "__default__";

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.process_cmdline(argc, argv);

  if (cfg.token() == default_initialization_marker) {
    std::random_device rd;
    produce_output(rd, cfg.iterations());
  } else {
    std::random_device rd(cfg.token());
    produce_output(rd, cfg.iterations());
  }
  return 0;
} catch (jb::usage const& ex) {
  std::cerr << "usage: " << ex.what() << std::endl;
  return ex.exit_status();
} catch (std::exception const& ex) {
  std::cerr << "standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "unknown exception raised" << std::endl;
  return 1;
}

namespace {
namespace defaults {
#ifndef JB_ITCH5_DEFAULT_check_random_device_token
#define JB_ITCH5_DEFAULT_check_random_device_token default_initialization_marker
#endif // JB_ITCH5_DEFAULT_check_random_device_token

#ifndef JB_ITCH5_DEFAULT_check_random_device_iterations
#define JB_ITCH5_DEFAULT_check_random_device_iterations 1000
#endif // JB_ITCH5_DEFAULT_check_random_device_iterations

std::string const token = JB_ITCH5_DEFAULT_check_random_device_token;
int const iterations = JB_ITCH5_DEFAULT_check_random_device_iterations;

} // namespace defaults

config::config()
    : iterations(
          desc("iterations")
              .help(
                  "Define how many values to extract from std::random_device."),
          this, defaults::iterations)
    , token(
          desc("token").help(
              "Define the parameter to initialize the random device."
              "The semantics are implementation defined, but on libc++ and "
              "libstd++ the value is the name of a device to read, such as "
              "'/dev/random' or '/dev/urandom'"),
          this, defaults::token) {
}

void config::validate() const {
  if (iterations() <= 0) {
    std::ostringstream os;
    os << "--iterations (" << iterations() << ") must be >= 0";
    throw jb::usage{os.str(), 1};
  }
}

} // namespace config
