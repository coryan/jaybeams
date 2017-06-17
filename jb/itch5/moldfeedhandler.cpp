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
#include <jb/itch5/mold_udp_channel.hpp>
#include <jb/itch5/process_iostream.hpp>
#include <jb/itch5/udp_receiver_config.hpp>
#include <jb/log.hpp>

#include <stdexcept>
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

  jb::config_attribute<config, jb::itch5::udp_receiver_config> primary;
  jb::config_attribute<config, jb::itch5::udp_receiver_config> secondary;
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
  if (cfg.port() == 0 or cfg.receive_address() == "") {
    return std::unique_ptr<jb::itch5::mold_udp_channel>();
  }
  return std::make_unique<jb::itch5::mold_udp_channel>(io, std::move(cb), cfg);
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
  config cfg;
  cfg.load_overrides(
      argc, argv, std::string("moldfeedhandler.yaml"), "JB_ROOT");
  jb::log::init(cfg.log());

  boost::asio::io_service io;

  using compute_book =
      jb::itch5::compute_book<jb::itch5::array_based_order_book>;
  using order_book = jb::itch5::order_book<jb::itch5::array_based_order_book>;
  auto cb =
      [](jb::itch5::message_header const& header,
         order_book const& updated_book, jb::itch5::book_update const& update) {
        std::cout << update.stock << " " << update.buy_sell_indicator << "\n";
      };

  compute_book handler(std::move(cb), cfg.book());

  auto process_buffer = [&handler](
      std::chrono::steady_clock::time_point recv_ts, std::uint64_t msgcnt,
      std::size_t msgoffset, char const* msgbuf, std::size_t msglen) {
    jb::itch5::process_buffer_mlist<compute_book, KNOWN_ITCH5_MESSAGES>::
        process(handler, recv_ts, msgcnt, msgoffset, msgbuf, msglen);
  };

  // TODO() - we need to refactor the mold_udp_channel class to
  // support multiple input sockets and to handle out-of-order,
  // duplicate, and gaps in the message stream.
  auto primary = create_udp_channel(io, process_buffer, cfg.primary());

  using endpoint = boost::asio::ip::tcp::endpoint;
  using address = boost::asio::ip::address;
  endpoint ep{address::from_string(cfg.control_host()), cfg.control_port()};

  // ... create a dispatcher to process the HTTP requests, register
  // some basic handlers ...
  using jb::ehs::request_type;
  using jb::ehs::response_type;
  auto dispatcher = std::make_shared<jb::ehs::request_dispatcher>("moldreplay");
  dispatcher->add_handler("/", [](request_type const&, response_type& res) {
    res.fields.insert("Content-type", "text/plain");
    res.body = "Server running...\r\n";
  });
  dispatcher->add_handler(
      "/config", [cfg](request_type const&, response_type& res) {
        res.fields.insert("Content-type", "text/plain");
        std::ostringstream os;
        os << cfg << "\r\n";
        res.body = os.str();
      });
  // ... we need to use a weak_ptr to avoid a cycle of shared_ptr ...
  std::weak_ptr<jb::ehs::request_dispatcher> disp = dispatcher;
  dispatcher->add_handler(
      "/metrics", [disp](request_type const&, response_type& res) {
        std::shared_ptr<jb::ehs::request_dispatcher> d(disp);
        if (not d) {
          res.status = 500;
          res.reason = beast::http::reason_string(res.status);
          res.body = std::string(
              "An internal error occurred\r\n"
              "Null request handler in /metrics\r\n");
          return;
        }
        res.fields.replace("content-type", "text/plain; version=0.0.4");
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
std::string const control_host = "0.0.0.0";
int const control_port = 23100;
std::string const primary_address = "127.0.0.1";
unsigned short const primary_port = 12300;
} // namespace defaults

config::config()
    : primary(
          desc("primary"), this, jb::itch5::udp_receiver_config()
                                     .receive_address(defaults::primary_address)
                                     .port(defaults::primary_port))
    , secondary(desc("secondary"), this)
    , control_host(
          desc("control-host")
              .help(
                  "Where does the server listen for control connections."
                  "Typically this is an address for the current host,"
                  "for example: 'localhost', '0.0.0.0', or '::1'."),
          this, defaults::control_host)
    , control_port(
          desc("control-port").help("The port to receive control connections."),
          this, defaults::control_port)
    , book(desc("book", "order-book-config"), this)
    , log(desc("log", "logging"), this) {
}

void config::validate() const {
  if (primary().port() == 0 and secondary().port() == 0) {
    throw jb::usage(
        "Either the primary or secondary port must be configured.", 1);
  }
  if (primary().receive_address() == "" and
      secondary().receive_address() == "") {
    throw jb::usage(
        "Either the primary or secondary receiving address must be configured.",
        1);
  }
  log().validate();
}

} // anonymous namespace
