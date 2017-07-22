/**
 * @file
 *
 * A Feed Handler for the ITCH-5.x protocol over MoldUDP.
 *
 * This program receives a ITCH-5.x feed over MoldUDP and generates
 * normalized inside messages for the feed.
 */
#include <jb/ehs/acceptor.hpp>
#include <jb/itch5/array_based_order_book.hpp>
#include <jb/itch5/generate_inside.hpp>
#include <jb/itch5/make_socket_udp_send.hpp>
#include <jb/itch5/mold_udp_channel.hpp>
#include <jb/itch5/process_iostream.hpp>
#include <jb/itch5/udp_receiver_config.hpp>
#include <jb/itch5/udp_sender_config.hpp>
#include <jb/mktdata/inside_levels_update.hpp>
#include <jb/fileio.hpp>
#include <jb/log.hpp>

#include <ctime>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

/**
 * Define types and functions used in this program.
 */
namespace {
/// Configuration parameters for moldfeedhandler
class config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config, int> levels;
  jb::config_attribute<config, jb::itch5::udp_receiver_config> primary;
  jb::config_attribute<config, jb::itch5::udp_receiver_config> secondary;
  jb::config_attribute<config, std::string> output_file;
  jb::config_attribute<config, std::vector<jb::itch5::udp_sender_config>>
      output;
  jb::config_attribute<config, std::string> control_host;
  jb::config_attribute<config, unsigned short> control_port;
  using book_config = typename jb::itch5::array_based_order_book::config;
  jb::config_attribute<config, book_config> book;
  jb::config_attribute<config, jb::log::config> log;
};

template <typename callback_t>
std::unique_ptr<jb::itch5::mold_udp_channel> create_udp_channel(
    boost::asio::io_service& io, callback_t cb,
    jb::itch5::udp_receiver_config const& cfg) {
  if (cfg.port() == 0 or cfg.address() == "") {
    return std::unique_ptr<jb::itch5::mold_udp_channel>();
  }
  return std::make_unique<jb::itch5::mold_udp_channel>(io, std::move(cb), cfg);
}

/// Define the type of order book used in the program.
using order_book = jb::itch5::order_book<jb::itch5::array_based_order_book>;

/// The output layer is composed of multiple instances of this
/// function type.
using output_function = std::function<void(
    jb::itch5::message_header const& header, order_book const& updated_book,
    jb::itch5::book_update const& update)>;

/// Create the output function for the file option.
output_function create_output_file(config const& cfg) {
  // ... otherwise create an output iostream and use it ...
  auto out = std::make_shared<boost::iostreams::filtering_ostream>();
  jb::open_output_file(*out, cfg.output_file());
  return [out](
      jb::itch5::message_header const& header, order_book const& updated_book,
      jb::itch5::book_update const& update) {
    auto bid = updated_book.best_bid();
    auto offer = updated_book.best_offer();
    *out << header.timestamp.ts.count() << " " << header.stock_locate << " "
         << update.stock << " " << bid.first.as_integer() << " " << bid.second
         << " " << offer.first.as_integer() << " " << offer.second << "\n";
  };
}

// TODO() - this value is cached, we need think about what happens for
// programs that run 24x7 ...
std::chrono::system_clock::time_point midnight() {
  using std::chrono::system_clock;
  // TODO - this should be ::localtime_r(), even though it is not part
  // of the C++ standard it is part of POSIX and better for threaded
  // environments ...
  auto now = std::time(nullptr);
  std::tm date = *std::localtime(&now);
  date.tm_sec = 0;
  date.tm_min = 0;
  date.tm_hour = 0;
  return system_clock::from_time_t(std::mktime(&date));
}

void send_inside_levels_update(
    boost::asio::ip::udp::socket& socket,
    boost::asio::ip::udp::endpoint const& destination,
    std::chrono::system_clock::time_point const& midnight,
    jb::itch5::message_header const& header, order_book const& updated_book,
    jb::itch5::book_update const& update) {
  // ... filter out messages that do not update the inside ...
  if (update.buy_sell_indicator == u'B') {
    if (updated_book.best_bid().first != update.px) {
      return;
    }
  } else {
    if (updated_book.best_offer().first != update.px) {
      return;
    }
  }
  // ... prepare the message to send ...
  // TODO() - the number of levels should be based on the "levels()"
  // configuration parameter
  jb::mktdata::inside_levels_update<1> msg;
  static_assert(
      std::is_pod<decltype(msg)>::value, "Message type should be a POD type");
  msg.message_type = jb::mktdata::inside_levels_update<1>::mtype;
  // TODO() - add configuration to send sizeof(msg) - sizeof(msg.annotations)
  msg.message_size = sizeof(msg);
  // TODO() - actually create sequence numbers ...
  msg.sequence_number = 0;
  // TODO() - this should be configured, the configuration parameters
  // should be the short strings (e.g. NASD-PITCH-5), and the feed
  // identifier should be looked up.
  msg.market.id = 0;
  msg.feed.id = 0;
  msg.feedhandler_ts.nanos =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::system_clock::now() - midnight)
          .count();
  // TODO() - another configuration parameter
  msg.source.id = 0;
  msg.exchange_ts.nanos = header.timestamp.ts.count();
  msg.feed_ts.nanos = header.timestamp.ts.count();
  // TODO() - this should be based on the JayBeams security id.
  msg.security.id = header.stock_locate;
  msg.bid_qty[0] = updated_book.best_bid().second;
  msg.bid_px[0] = updated_book.best_bid().first.as_integer();
  msg.offer_qty[0] = updated_book.best_offer().second;
  msg.offer_px[0] = updated_book.best_offer().first.as_integer();
  // TODO() - this should be based on configuration parameters, and
  // range checked ...
  std::memcpy(msg.annotations.mic, "NASD", 4);
  std::memcpy(msg.annotations.feed_name, "NASD-PITCH-5x", 13);
  std::memcpy(msg.annotations.source_name, "NASD-PITCH-5x", 13);
  // TODO() - NASDAQ data is mostly normalized, some NYSE securities
  // have a different ticker in NASDAQ data vs. CQS and NYSE data.
  std::memcpy(
      msg.annotations.security_normalized, update.stock.c_str(),
      update.stock.wire_size);
  std::memcpy(
      msg.annotations.security_feed, update.stock.c_str(),
      update.stock.wire_size);
  // TODO() - consider a non-blocking write for the socket
  socket.send_to(
      boost::asio::buffer(&msg, msg.message_size.value()), destination);
  // TODO() - increment a counter to show that the socket was sent,
  // different counters for success and failure ...
}

/// Create an output function for a single socket
output_function create_output_socket(
    boost::asio::io_service& io, jb::itch5::udp_sender_config const& cfg) {
  auto s = jb::itch5::make_socket_udp_send<>(io, cfg);
  auto socket = std::make_shared<decltype(s)>(std::move(s));
  auto const address = boost::asio::ip::address::from_string(cfg.address());
  auto const destination = boost::asio::ip::udp::endpoint(address, cfg.port());
  auto const mid = midnight();
  return [socket, cfg, destination, mid](
      jb::itch5::message_header const& h, order_book const& ub,
      jb::itch5::book_update const& u) {
    send_inside_levels_update(*socket, destination, mid, h, ub, u);
  };
}

/// Create a composite output function aggregating all the different
/// configured outputs
output_function
create_output_layer(boost::asio::io_service& io, config const& cfg) {
  std::vector<output_function> outs;
  if (cfg.output_file() != "") {
    outs.push_back(create_output_file(cfg));
  }
  for (auto const& outcfg : cfg.output()) {
    if (outcfg.port() == 0 and outcfg.address() == "") {
      continue;
    }
    outs.push_back(create_output_socket(io, outcfg));
  }
  return [outputs = std::move(outs)](
      jb::itch5::message_header const& header, order_book const& updated_book,
      jb::itch5::book_update const& update) {
    for (auto f : outputs) {
      f(header, updated_book, update);
    }
  };
}

} // anonymous namespace

#define KNOWN_ITCH5_MESSAGES                                                   \
  jb::itch5::add_order_message, jb::itch5::add_order_mpid_message,             \
      jb::itch5::broken_trade_message, jb::itch5::cross_trade_message,         \
      jb::itch5::ipo_quoting_period_update_message,                            \
      jb::itch5::market_participant_position_message,                          \
      jb::itch5::mwcb_breach_message, jb::itch5::mwcb_decline_level_message,   \
      jb::itch5::net_order_imbalance_indicator_message,                        \
      jb::itch5::order_cancel_message, jb::itch5::order_delete_message,        \
      jb::itch5::order_executed_message,                                       \
      jb::itch5::order_executed_price_message,                                 \
      jb::itch5::order_replace_message,                                        \
      jb::itch5::reg_sho_restriction_message,                                  \
      jb::itch5::stock_directory_message,                                      \
      jb::itch5::stock_trading_action_message,                                 \
      jb::itch5::system_event_message, jb::itch5::trade_message

int main(int argc, char* argv[]) try {
  // All JayBeam programs read their configuration from a YAML file,
  // the values can be overriden by the command-line arguments, but it
  // is not recommended to set all the values via command-line flags
  // ...
  // TODO() - make it possible to read the YAML file from a etcd
  // path.  That way we can keep all the configurations in a single
  // place ...
  config cfg;
  cfg.load_overrides(
      argc, argv, std::string("moldfeedhandler.yaml"), "JB_ROOT");
  jb::log::init(cfg.log());

  // ... this program basically has a single control loop.  A future
  // version should separate performance critical code to its own
  // threads with their own io_service ...
  boost::asio::io_service io;

  // ... define the classes used to build the book ...
  using compute_book =
      jb::itch5::compute_book<jb::itch5::array_based_order_book>;

  // ... the data path is implemented as a series of stages, each one
  // calls the next using lambdas.  The last lambda to be called --
  // where the data is sent to a file or a socket -- is the first to
  // be constructed ...
  // TODO() - actually output the messages to UDP sockets and files
  // TODO() - run a master election via etcd and only output to
  // sockets if this is the master
  auto output_layer = create_output_layer(io, cfg);

  // ... here we should have a layer to arbitrage between the ITCH-5.x
  // feed and the UQDF/CQS feeds.  Normally ITCH-5.x is a better feed,
  // richer data, more accurate, and lower latency.  But the ITCH-5.x
  // feed depends on never losing a message.  When you do, there are
  // multiple alternatives (e.g. requesting a retransmission from the
  // exchange, using a sync+tell feed from the exchange).  We propose
  // to fallback to the UQDF/CQS feeds, which are stateless.  The
  // recovery using those feeds is almost immediate.
  // The ITCH-5.x book can be cleared and rebuilt using only new
  // messages, for most tickers the freshly constructed book is
  // accurate enough within seconds.  Switching back to ITCH-5.x after
  // falling back to UQDF/CQS will require detecting when the two
  // feeds are synchronized again ...
  // TODO() - implement all the fallback / recovery complexity ...

  // ... in this layer we compute the book, i.e., assemble the list of
  // orders received from the feed into a quantity at each price level
  // ...
  compute_book book_build_layer(std::move(output_layer), cfg.book());

  // ... in this layer we decode the raw ITCH messages into objects
  // that can be more easily manipulated ...
  // TODO() - we need to break out the non-book-building messages and
  // bypass the book_build_layer for them, send them directly to the
  // output layer.  Or maybe have a separate output layer for
  // non-book-build messages, which can be running at lower priority
  // ...
  auto itch_decoding_layer = [&book_build_layer](
      std::chrono::steady_clock::time_point recv_ts, std::uint64_t msgcnt,
      std::size_t msgoffset, char const* msgbuf, std::size_t msglen) {
    jb::itch5::process_buffer_mlist<compute_book, KNOWN_ITCH5_MESSAGES>::
        process(book_build_layer, recv_ts, msgcnt, msgoffset, msgbuf, msglen);
  };

  /// ... here we are missing a layer to arbitrage between the two UDP
  // message sources, something like ...
  //   auto sequencing_layer = [&itch_decoding_layer](...) {};
  // TODO() - we need to refactor the mold_udp_channel class to
  // support multiple input sockets and to handle out-of-order,
  // duplicate, and gaps in the message stream.
  auto data_source_layer =
      create_udp_channel(io, itch_decoding_layer, cfg.primary());

  // ... that was it for the critical data path.  There are several
  // TODO() entries there ...

  // In this section we create the control and monitoring path for the
  // application.  The control and monitoring path is implemented by a
  // HTTP server that responds to simple GET requests.  Adding new
  // control methods is easy, as we will see ...
  // TODO() - this should be refactored to a "application" class, they
  // are very repetitive.  We need to solve the counter problem first.
  using endpoint = boost::asio::ip::tcp::endpoint;
  using address = boost::asio::ip::address;
  endpoint ep{address::from_string(cfg.control_host()), cfg.control_port()};

  // ... the request and response types are simple in-memory strings,
  // this is suitable for our purposes as the payloads will generally
  // be small ...
  using jb::ehs::request_type;
  using jb::ehs::response_type;
  // ... this object keeps track of all the handlers for each "path"
  // in the HTTP request.  The application registers a number of
  // handlers for monitoring and control ...
  auto dispatcher = std::make_shared<jb::ehs::request_dispatcher>("moldreplay");
  // ... the root serves simply as a indication that the server is
  // running ...
  dispatcher->add_handler("/", [](request_type const&, response_type& res) {
    res.insert("Content-type", "text/plain");
    res.body = "Server running...\r\n";
  });
  // ... this prints out the system configuration (command-line
  // parameters and the YAML configuration file), in YAML format ...
  dispatcher->add_handler(
      "/config", [cfg](request_type const&, response_type& res) {
        res.insert("Content-type", "text/plain");
        std::ostringstream os;
        os << cfg << "\r\n";
        res.body = os.str();
      });
  // ... we need to use a weak_ptr to avoid a cycle of shared_ptr ...
  std::weak_ptr<jb::ehs::request_dispatcher> disp = dispatcher;
  // ... this handler collects the metrics and reports them in human
  // readable form ...
  // TODO() - we need a separate handler to serve the metrics in
  // protobuf form for efficiency
  // TODO() - once we solve the counter problem we should show the
  // counter values here, not just whatever the dispatcher collects
  // about itself
  dispatcher->add_handler(
      "/metrics", [disp](request_type const&, response_type& res) {
        std::shared_ptr<jb::ehs::request_dispatcher> d(disp);
        if (not d) {
          res.result(beast::http::status::internal_server_error);
          res.body = std::string("An internal error occurred\r\n"
                                 "Null request handler in /metrics\r\n");
          return;
        }
        res.set("content-type", "text/plain; version=0.0.4");
        d->append_metrics(res);
      });

  // ... create an acceptor to handle incoming connections, if we wanted
  // to, we could create multiple acceptors on different addresses
  // pointing to the same dispatcher ...
  jb::ehs::acceptor acceptor(io, ep, dispatcher);

  // ... run the program forever ...
  // TODO() - we should be able to gracefully terminate the program
  // with a handler in the embedded http server, and/or with a signal
  io.run();

  return 0;
} catch (jb::usage const& u) {
  std::cerr << u.what() << std::endl;
  return u.exit_status();
} catch (std::exception const& ex) {
  std::cerr << "Standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "Unknown exception raised" << std::endl;
  return 1;
}

namespace {
namespace defaults {
int const levels = 4;
std::string const control_host = "0.0.0.0";
int const control_port = 23100;
std::string const mold_address = "127.0.0.1";
unsigned short const mold_port = 12300;
std::string const output_address = "127.0.0.1";
unsigned short const output_port = 13000;
} // namespace defaults

config::config()
    : levels(
          desc("levels").help("Configure the number of levels generated by "
                              "this feed handler.  The only allowed values are "
                              "1, 4, or 8."),
          this, defaults::levels)
    , primary(
          desc("primary"), this, jb::itch5::udp_receiver_config()
                                     .address(defaults::mold_address)
                                     .port(defaults::mold_port))
    , secondary(
          desc("secondary"), this,
          jb::itch5::udp_receiver_config().address(defaults::mold_address))
    , output_file(
          desc("output-file")
              .help("Configure the feed handler to log to a "
                    "ASCII (possibly compressed) file."
                    "  The user should consider the performance impact of this "
                    "option when using this as the primary feedhandler."),
          this)
    , output(
          desc("output").help(
              "Configure the output UDP addresses for the feed handler "
              "messages."
              "  Typically one output UDP address is enough, the application "
              "can be configured with multiple output sockets for network "
              "redundancy, or to send copies to another process in "
              "the localhost for logging."),
          this)
    , control_host(
          desc("control-host")
              .help("Where does the server listen for control connections."
                    "Typically this is an address for the current host,"
                    "for example: 'localhost', '0.0.0.0', or '::1'."),
          this, defaults::control_host)
    , control_port(
          desc("control-port").help("The port to receive control connections."),
          this, defaults::control_port)
    , book(desc("book", "order-book-config"), this)
    , log(desc("log", "logging"), this) {
  output({jb::itch5::udp_sender_config()
              .address(defaults::output_address)
              .port(defaults::output_port)});
}

void config::validate() const {
  if (levels() != 1 and levels() != 4 and levels() != 8) {
    std::ostringstream os;
    os << "Invalid value (" << levels() << ") for --levels option.";
    throw jb::usage(os.str(), 1);
  }
  if (primary().port() == 0 and secondary().port() == 0) {
    throw jb::usage(
        "Either the primary or secondary port must be configured.", 1);
  }
  if (primary().address() == "" and secondary().address() == "") {
    throw jb::usage(
        "Either the primary or secondary receiving address must be configured.",
        1);
  }
  int cnt = 0;
  int outputs = 0;
  for (auto const& outcfg : output()) {
    if ((outcfg.port() != 0 and outcfg.address() == "") or
        (outcfg.port() == 0 and outcfg.address() != "")) {
      std::ostringstream os;
      os << "Partially configured output socket #" << cnt << " ("
         << outcfg.address() << " / " << outcfg.port() << ")";
      throw jb::usage(os.str(), 1);
    }
    if (outcfg.port() != 0 and outcfg.address() != "") {
      ++outputs;
    }
  }
  if (outputs == 0 and output_file() == "") {
    throw jb::usage("No --output nor --output-file configured", 1);
  }
  log().validate();
}

} // anonymous namespace
