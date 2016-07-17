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
  typedef std::function<void(
      std::chrono::steady_clock::time_point, std::uint64_t, std::size_t,
      char const*, std::size_t)> buffer_handler;
  
  mold_channel(
      buffer_handler handler,
      boost::asio::io_service& io,
      std::string const& listen_address,
      int multicast_port,
      std::string const& multicast_group);

 private:
  void restart_async_receive_from();
  void handle_received(
      boost::system::error_code const& ec, size_t bytes_received);

 private:
  buffer_handler handler_;
  boost::asio::ip::udp::socket socket_;
  std::uint64_t expected_sequence_number_;
  std::size_t message_offset_;

  static std::size_t const buflen = 1<<16;
  char buffer_[buflen];
  boost::asio::ip::udp::endpoint sender_endpoint_;
};

} // anonymous namespace

#define KNOWN_ITCH5_MESSAGES                    \
  jb::itch5::add_order_message,                 \
  jb::itch5::add_order_mpid_message, \
  jb::itch5::broken_trade_message, \
  jb::itch5::cross_trade_message, \
  jb::itch5::ipo_quoting_period_update_message, \
  jb::itch5::market_participant_position_message, \
  jb::itch5::mwcb_breach_message, \
  jb::itch5::mwcb_decline_level_message, \
  jb::itch5::net_order_imbalance_indicator_message, \
  jb::itch5::order_cancel_message, \
  jb::itch5::order_delete_message, \
  jb::itch5::order_executed_message, \
  jb::itch5::order_executed_price_message, \
  jb::itch5::order_replace_message, \
  jb::itch5::reg_sho_restriction_message, \
  jb::itch5::stock_directory_message, \
  jb::itch5::stock_trading_action_message, \
  jb::itch5::system_event_message, \
  jb::itch5::trade_message

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.load_overrides(
      argc, argv, std::string("mold2inside.yaml"), "JB_ROOT");
  jb::log::init(cfg.log());

  boost::asio::io_service io_service;

  boost::iostreams::filtering_ostream out;
  jb::open_output_file(out, cfg.output_file());

  std::map<jb::itch5::stock_t, jb::offline_feed_statistics> per_symbol;
  jb::offline_feed_statistics stats(cfg.stats());

  auto cb = [&stats, &cfg, &out, &per_symbol](
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
  auto process_buffer = [&handler](
      std::chrono::steady_clock::time_point recv_ts,
      std::uint64_t msgcnt, std::size_t msgoffset,
      char const* msgbuf, std::size_t msglen) {
    jb::itch5::process_buffer_mlist<
          jb::itch5::compute_inside,KNOWN_ITCH5_MESSAGES>::process(
        handler, recv_ts, msgcnt, msgoffset, msgbuf, msglen);
  };

  mold_channel channel(
      process_buffer, io_service, cfg.listen_address(), cfg.multicast_port(),
      cfg.multicast_group());

  io_service.run();

  jb::offline_feed_statistics::print_csv_header(std::cout);
  for (auto const& i : per_symbol) {
    i.second.print_csv(i.first.c_str(), std::cout);
  }
  stats.print_csv("__aggregate__", std::cout);


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
  return "::1";
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
    buffer_handler handler,
    boost::asio::io_service& io,
    std::string const& listen_address,
    int multicast_port,
    std::string const& multicast_group)
    : handler_(handler)
    , socket_(io)
    , expected_sequence_number_(0)
    , message_offset_(0) {
  auto address = boost::asio::ip::address::from_string(listen_address);
  boost::asio::ip::udp::endpoint endpoint(address, multicast_port);
  boost::asio::ip::udp::socket socket(io);
  socket_.open(endpoint.protocol());
  socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));

  JB_LOG(info) << "Requested endpoint=" << endpoint;
  auto group_address = boost::asio::ip::address::from_string(multicast_group);
  if (group_address.is_multicast()) {
    socket_.set_option(boost::asio::ip::multicast::join_group(group_address));
    socket_.set_option(boost::asio::ip::multicast::enable_loopback(true));
    JB_LOG(info) << " .. joining multicast group=" << group_address;
  } else {
    socket_.bind(endpoint);
    JB_LOG(info) << " .. bound to endpoint=" << socket_.local_endpoint();
  }

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
    // Fetch the current timestamp, all the messages in the
    // MoldUDP64 packet share the same timestamp ...
    auto recv_ts = std::chrono::steady_clock::now();
    // ... fetch the sequence number of the first message in the
    // MoldUDP64 packet ...
    auto sequence_number = jb::itch5::decoder<true,std::uint64_t>::r(
        bytes_received, buffer_,
        jb::itch5::mold_udp_protocol::sequence_number_offset);
    // ... and fetch the number of blocks in the MoldUDP64 packet ...
    auto block_count = jb::itch5::decoder<true,std::uint16_t>::r(
        bytes_received, buffer_,
        jb::itch5::mold_udp_protocol::block_count_offset);

    JB_LOG(info) << "Received packet with starting seqno=" << sequence_number;
    // ... if the message is out of order we simply print the problem,
    // in a more realistic application we would need to reorder them
    // and gap fill if needed, and sometimes do even more complicated
    // things ...
    if (sequence_number != expected_sequence_number_) {
      JB_LOG(info) << "Mismatched sequence number, expected="
                   << expected_sequence_number_
                   << ", got=" << sequence_number;
    }

    

    //  offset represents the start of the MoldUDP64 block ...
    std::size_t offset = jb::itch5::mold_udp_protocol::block_count_offset + 2;
    // ... process each message in the MoldUDP64 packet, in order.
    for (std::size_t block = 0; block != block_count; ++block) {
      // ... fetch the block size ...
      auto message_size = jb::itch5::decoder<true,std::uint16_t>::r(
          bytes_received, buffer_, offset);
      // ... increment the offset into the MoldUDP64 packet, this is
      // the start of the ITCH-5.x message ...
      offset += 2;
      // ... process the buffer ...
      handler_(
          recv_ts, expected_sequence_number_, message_offset_,
          buffer_ + offset, message_size);

      // ... increment counters to reflect that this message was
      // proceesed ...
      sequence_number++;
      message_offset_ += message_size;
    }
    // ... since we are not dealing with gaps, or message reordering
    // just reset the next expected number ...
    expected_sequence_number_ = sequence_number;
  }
  restart_async_receive_from();
}

} // anonymous namespace
