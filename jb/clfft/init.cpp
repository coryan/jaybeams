#include "jb/clfft/init.hpp"
#include <jb/clfft/error.hpp>

jb::clfft::init::init() {
  clfftSetupData data{};
  auto err = clfftSetup(&data);
  jb::clfft::check_error_code(err, "clfftSetup");
}

jb::clfft::init::~init() {
  auto err = clfftTeardown();
  jb::clfft::check_error_code(err, "clfftTeardown");
}
