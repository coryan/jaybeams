#include <jb/itch5/compute_inside.hpp>
#include <jb/itch5/process_iostream.hpp>
#include <jb/offline_feed_statistics.hpp>
#include <jb/fileio.hpp>
#include <jb/log.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>

#include <iostream>
#include <stdexcept>
#include <unordered_map>

namespace {

class config : public jb::config_object {
 public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config,int> multicast_port;
  jb::config_attribute<config,std::string> listen_address;
  jb::config_attribute<config,std::string> multicast_group;
  jb::config_attribute<config,std::string> output_file;
  jb::config_attribute<config,jb::log::config> log;
  jb::config_attribute<config,jb::offline_feed_statistics::config> stats;
  jb::config_attribute<config,jb::offline_feed_statistics::config> symbol_stats;
  jb::config_attribute<config,bool> enable_symbol_stats;
};

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.load_overrides(
      argc, argv, std::string("mold2inside.yaml"), "JB_ROOT");
  jb::log::init(cfg.log());

  boost::asio::io_service io_service;

  auto address = boost::asio::ip::address::from_string(cfg.listen_address());
  boost::asio::ip::udp::endpoint endpoint(address, cfg.multicast_port());
  boost::asio::ip::udp::socket socket(io_service, endpoint);
  JB_LOG(info) << "Listening on endpoint=" << endpoint
               << ", local_endpoint=" << socket.local_endpoint();

#if 0
  udp::endpoint listen_endpoint(
      boost::asio::ip::address::from_string("0.0.0.0"), tokens[1]);
  auto address = boost::asio::ip::address::from_string(tokens[0]);

  udp::socket s(io_service);
  s.open(listen_endpoint.protocol());
  s.set_option(socket::reuse_address(true));
  s.bind(listen_endpoint);
  s.set_option(boost::asio::ip::multicast::join_group(address));

  std::size_t const max_udp_length = 1<<16;
  char buffer[max_udp_length];

  boost::asio::ip::udp sender_endpoint;
  auto receive_callback = [&s](
      boost::system::error_coder& error,
      size_t bytes_received) {
  };
  s.async_receive_from(
      boost::asio::buffer(buffer, max_udp_length), sender_endpoint,
      [receive_callback](boost::system::error_coder& error,
                         size_t bytes_received) {
        received_callback(error, bytes_received);
        s.async_receive_from(
            boost::asio::buffer(buffer, max_udp_length), sender_endpoint,
            
      });
  
  boost::iostreams::filtering_ostream out;
  jb::open_output_file(out, cfg.output_file());

  std::map<jb::itch5::stock_t, jb::offline_feed_statistics> per_symbol;
  jb::offline_feed_statistics stats(cfg.stats());

  auto cb = [&](
      jb::itch5::compute_inside::time_point recv_ts,
      jb::itch5::message_header const& header,
      jb::itch5::stock_t const& stock,
      jb::itch5::half_quote const& bid,
      jb::itch5::half_quote const& offer) {
    auto pl = std::chrono::steady_clock::now() - recv_ts;
    stats.sample(header.timestamp.ts, pl);

    if (cfg.enable_symbol_stats()) {
      auto i = per_symbol.find(stock);
      if (i == per_symbol.end()) {
        auto p = per_symbol.emplace(
            stock, jb::offline_feed_statistics(cfg.symbol_stats()));
        i = p.first;
      }
      i->second.sample(header.timestamp.ts, pl);
    }

    out << header.timestamp.ts.count()
        << " " << header.stock_locate
        << " " << stock
        << " " << bid.first.as_integer()
        << " " << bid.second
        << " " << offer.first.as_integer()
        << " " << offer.second
        << "\n";
  };

  jb::itch5::compute_inside handler(cb);
  jb::itch5::process_iostream(in, handler);

  jb::offline_feed_statistics::print_csv_header(std::cout);
  for (auto const& i : per_symbol) {
    i.second.print_csv(i.first.c_str(), std::cout);
  }
  stats.print_csv("__aggregate__", std::cout);
#endif

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

// Define the default per-symbol stats
jb::offline_feed_statistics::config default_per_symbol_stats() {
  return jb::offline_feed_statistics::config()
      .reporting_interval_seconds(24 * 3600) // effectively disable updates
      .max_processing_latency_nanoseconds(10000) // limit memory usage
      .max_interarrival_time_nanoseconds(10000)  // limit memory usage 
      .max_messages_per_microsecond(1000)  // limit memory usage
      .max_messages_per_millisecond(10000) // limit memory usage
      .max_messages_per_second(10000)      // limit memory usage
      ;
}

std::string default_listen_address() {
  return "::";
}

std::string default_multicast_group() {
  return "FF01::1";
}

int default_multicast_port() {
  return 50000;
}

config::config()
    : multicast_port(desc("multicast-port").help(
        "The multicast port to listen in."), this, default_multicast_port())
    , listen_address(desc("listen-address").help(
        "The address to listen in, typically 0.0.0.0, ::, or a specific "
        "NIC address."), this, default_listen_address())
    , multicast_group(desc("multicast-group").help(
        "The multicast group carrying the MOLD data."), this,
                      default_multicast_group())
    , output_file(desc("output-file").help(
        "The name of the file where to store the inside data."
        "  Files ending in .gz are automatically compressed."), this)
    , log(desc("log", "logging"), this)
    , stats(desc("stats", "offline-feed-statistics"), this)
    , symbol_stats(desc("symbol-stats", "offline-feed-statistics"),
                   this, default_per_symbol_stats())
    , enable_symbol_stats(
        desc("enable-symbol-stats").help(
            "If set, enable per-symbol statistics."
            "  Collecting per-symbol statistics is expensive in both"
            " memory and execution time, so it is disabled by default."),
        this, false)
{}

void config::validate() const {
  if (output_file() == "") {
    throw jb::usage(
        "Missing output-file setting."
        "  You must specify an output file.", 1);
  }
  log().validate();
  stats().validate();
  symbol_stats().validate();
}

} // anonymous namespace
