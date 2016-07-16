#include <jb/itch5/compute_inside.hpp>
#include <jb/itch5/process_iostream.hpp>
#include <jb/itch5/mold_udp_protocol_constants.hpp>
#include <jb/offline_feed_statistics.hpp>
#include <jb/fileio.hpp>
#include <jb/log.hpp>

#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/ip/multicast.hpp>

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

class mold_channel {
 public:
  mold_channel(
      boost::asio::io_service& io,
      std::string const& listen_address,
      int multicast_port,
      std::string const& multicast_group);

 private:
  void restart_async_receive_from();
  void handle_received(
      boost::system::error_code const& ec, size_t bytes_received);

 private:
  boost::asio::ip::udp::socket socket_;
  std::uint64_t expected_sequence_number_;

  static std::size_t const buflen = 1<<16;
  char buffer_[buflen];
  boost::asio::ip::udp::endpoint sender_endpoint_;
};

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.load_overrides(
      argc, argv, std::string("mold2inside.yaml"), "JB_ROOT");
  jb::log::init(cfg.log());

  boost::asio::io_service io_service;

  mold_channel channel(
      io_service, cfg.listen_address(), cfg.multicast_port(),
      cfg.multicast_group());

  io_service.run();

#if 0
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

mold_channel::mold_channel(
    boost::asio::io_service& io,
    std::string const& listen_address,
    int multicast_port,
    std::string const& multicast_group)
    : socket_(io)
    , expected_sequence_number_(0) {
  auto address = boost::asio::ip::address::from_string(listen_address);
  boost::asio::ip::udp::endpoint endpoint(address, multicast_port);
  boost::asio::ip::udp::socket socket(io);
  socket_.open(endpoint.protocol());
  socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
  socket_.bind(endpoint);

  auto group_address = boost::asio::ip::address::from_string(multicast_group);
  socket_.set_option(boost::asio::ip::multicast::join_group(group_address));

  JB_LOG(info) << "Listening on endpoint=" << socket_.local_endpoint()
               << ", requested=" << endpoint
               << " for data in multicast group=" << group_address;

  restart_async_receive_from();
}

void mold_channel::restart_async_receive_from() {
  socket_.async_receive_from(
      boost::asio::buffer(buffer_, buflen), sender_endpoint_,
      [this](boost::system::error_code const& ec, size_t bytes_received) {
        handle_received(ec, bytes_received);
      });
}

void mold_channel::handle_received(
    boost::system::error_code const& ec, size_t bytes_received) {
  if (!ec and bytes_received > 0) {
    auto sequence_number = jb::itch5::decoder<true,std::uint64_t>::r(
        bytes_received, buffer_,
        jb::itch5::mold_udp_protocol::sequence_number_offset);

    auto block_count = jb::itch5::decoder<true,std::uint16_t>::r(
        bytes_received, buffer_,
        jb::itch5::mold_udp_protocol::block_count_offset);

    if (sequence_number != expected_sequence_number_) {
      JB_LOG(info) << "Mismatched sequence number, expected="
                   << expected_sequence_number_
                   << ", got=" << sequence_number;
    }
    expected_sequence_number_ += sequence_number + block_count;
  }
  restart_async_receive_from();
}

} // anonymous namespace
