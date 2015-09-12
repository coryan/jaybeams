#include <jb/itch5/compute_inside.hpp>
#include <jb/itch5/process_iostream.hpp>
#include <jb/fileio.hpp>
#include <jb/log.hpp>

#include <iostream>
#include <stdexcept>
#include <unordered_map>

namespace {

class config : public jb::config_object {
 public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config,std::string> input_file;
  jb::config_attribute<config,jb::log::config> log;
};


} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.load_overrides(
      argc, argv, std::string("itch5_inside.yaml"), "JB_ROOT");
  jb::log::init(cfg.log());

  boost::iostreams::filtering_istream in;
  jb::open_input_file(in, cfg.input_file());

  auto cb = [](jb::itch5::compute_inside::time_point,
               jb::itch5::stock_t,
               jb::itch5::half_quote const&,
               jb::itch5::half_quote const&) {};

  jb::itch5::compute_inside handler(cb);
  jb::itch5::process_iostream(in, handler);

  return 0;
} catch(jb::usage const& u) {
  std::cerr << u.what() << std::endl;
  return u.exit_status();
} catch(std::exception const& ex) {
  std::cerr << "Standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch(...) {
  std::cerr << "Unknown exception raised" << std::endl;
  return 1;
}

namespace {
config::config()
    : input_file(desc("input-file").help(
        "An input file with ITCH-5.0 messages."), this)
    , log(desc("log"), this)
{}

void config::validate() const {
  if (input_file() == "") {
    throw jb::usage(
        "Missing input-file setting."
        "  You must specify an input file.", 1);
  }
  log().validate();
}

} // anonymous namespace
