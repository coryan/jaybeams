/**
 * @file
 *
 * Generate some output from std::random_device.
 *
 * I am interested in examining the output from std::random_device to
 * evaluate its statistical strength.
 */
#include <jb/testing/initialize_mersenne_twister.hpp>
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
  jb::config_attribute<config, unsigned int> seed;
};

void produce_output(config const& cfg) {
  auto generator = jb::testing::initialize_mersenne_twister<std::mt19937_64>(
      cfg.seed(), cfg.token());

  for (int i = 0; i != cfg.iterations(); ++i) {
    std::cout << std::uniform_real_distribution<>(0, 1)(generator) << "\n";
  }
}

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.process_cmdline(argc, argv);

  produce_output(cfg);
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
#define JB_ITCH5_DEFAULT_check_random_device_token                             \
  jb::testing::default_initialization_marker
#endif // JB_ITCH5_DEFAULT_check_random_device_token

#ifndef JB_ITCH5_DEFAULT_check_random_device_iterations
#define JB_ITCH5_DEFAULT_check_random_device_iterations 10000
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
          this, defaults::token)
    , seed(
          desc("seed").help(
              "If non-zero the generator is initialized using this seed."),
          this, 0) {
}

void config::validate() const {
  if (iterations() <= 0) {
    std::ostringstream os;
    os << "--iterations (" << iterations() << ") must be >= 0";
    throw jb::usage{os.str(), 1};
  }
}

} // namespace config
