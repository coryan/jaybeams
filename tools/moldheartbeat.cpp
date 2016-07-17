#include <jb/itch5/mold_udp_pacer.hpp>
#include <jb/log.hpp>
#include <jb/as_hhmmss.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/ip/multicast.hpp>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

#include <net/if.h>

namespace {

class config : public jb::config_object {
 public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config,std::string> destination;
  jb::config_attribute<config,std::string> port;
  jb::config_attribute<config,jb::log::config> log;
  jb::config_attribute<config,jb::itch5::mold_udp_pacer_config> pacer;
};

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.load_overrides(
      argc, argv, std::string("moldheartbeat.yaml"), "JB_ROOT");
  jb::log::init(cfg.log());

  boost::asio::io_service io_service;
  boost::asio::ip::udp::resolver resolver(io_service);
  boost::asio::ip::udp::resolver::query query(cfg.destination(), cfg.port());

  boost::asio::ip::udp::endpoint endpoint;
  auto i = resolver.resolve(query);
  if (i == boost::asio::ip::udp::resolver::iterator()) {
    JB_LOG(error) << "Cannot resolve destination address or port"
                  << ", address=" << cfg.destination()
                  << ", port=" << cfg.port();
    throw std::runtime_error("cannot resolve destination address or port");
  }
  endpoint = *i;
  
  JB_LOG(info) << "Sending to endpoint=" << endpoint;
  boost::asio::ip::udp::socket socket(io_service, endpoint.protocol());
  socket.set_option(boost::asio::ip::multicast::enable_loopback(true));

  jb::itch5::mold_udp_pacer<> pacer(cfg.pacer());
  auto sink = [&socket, &endpoint](auto buffers) {
    socket.send_to(buffers, endpoint);
  };
  for (int i = 0; i != 10000; ++i) {
    if (i % 100 == 0) {
      JB_LOG(info) << "Sending hearbeat # " << i;
    }
    pacer.heartbeat(sink);
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  return 0;
} catch(jb::usage const& u) {
  std::cout << u.what() << std::endl;
  return u.exit_status();
} catch(std::exception const& ex) {
  std::cerr << "Standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch(...) {
  std::cerr << "Unknown exception raised" << std::endl;
  return 1;
}

namespace {

std::string default_udp_port() {
  return "50000";
}

std::string default_destination() {
  return "::1";
}

config::config()
    : destination(desc("destination").help(
        "The destination for the UDP messages. "
        "The destination can be a unicast or multicast address."), this,
                  default_destination())
    , port(desc("port").help(
        "The destination port for the UDP messages. "), this,
           default_udp_port())
    , log(desc("log", "logging"), this)
    , pacer(desc("pacer", "mold-udp-pacer"), this)
{}

void config::validate() const {
  log().validate();
  pacer().validate();
}

} // anonymous namespace

