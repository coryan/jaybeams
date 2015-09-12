#include <jb/itch5/process_iostream.hpp>
#include <jb/fileio.hpp>
#include <jb/log.hpp>

#include <iostream>
#include <functional>
#include <map>
#include <stdexcept>
#include <unordered_map>

namespace jb {
namespace itch5 {

typedef std::pair<price4_t, int> half_quote;

/**
 * Maintain the ITCH-5.0 order book for a single security.
 *
 * ITCH-5.0, like other market data feeds provide order-by-order
 * detail, that is, the feed includes a message for each order
 * received by the exchange, as well as the changes to these 
 * orders, i.e. when the execute, when their quantity (and/or price)
 * is modified, and when they are canceled..  Such feeds are sometimes
 * called Level III feeds.  Typically only orders that do not
 * immediately execute in full are included in the feed.
 *
 * There is substantial variation in the format of the messages, and
 * some variation as to whether executions are represented differently
 * than a partial cancel, and whether changes in price are allowed or
 * create new order ids.
 *
 * This class receives a stream of (unnormalized) ITCH-5.0 messages
 * for a single security, and organizes the orders in a
 * book, i.e. a  data structure where orders at the same price are
 * grouped together, and one can quickly ask:
 *
 * - What is the best bid (highest price of BUY orders) and what is
 * the total quantity available at that price?
 * - What is the best offer (lowest price of SELL orders) and what is
 * the total quantity available at that price?
 *
 * The users of this class can provide a callback functor to notify
 * them of changes to the inside.
 */
class order_book {
 public:
  order_book() {}

  half_quote best_bid() const {
    if (buy_.empty()) {
      return half_quote(price4_t(0), 0);
    }
    auto i = buy_.begin();
    return half_quote(i->first, i->second);
  }

  half_quote best_offer() const {
    if (sell_.empty()) {
      return half_quote(price4_t(2000000000UL), 0);
    }
    auto i = sell_.begin();
    return half_quote(i->first, i->second);
  }

  void handle_add(buy_sell_indicator_t side, price4_t px, int qty) {
    if (side == buy_sell_indicator_t('B')) {
      auto p = buy_.emplace(px, 0);
      p.first->second += qty;
    } else {
      auto p = sell_.emplace(px, 0);
      p.first->second += qty;
    }
  }

 private:
  typedef std::map<price4_t,int,std::greater<price4_t>> buys;
  typedef std::map<price4_t,int,std::less<price4_t>> sells;

  buys buy_;
  sells sell_;
};

/**
 * An implementation of jb::message_handler_concept to compute the
 * inside.
 */
class inside_handler {
 public:
  inside_handler()
  {}

  typedef std::chrono::steady_clock::time_point time_point;

  time_point now() const {
    return std::chrono::steady_clock::now();
  }

  template<typename message_type>
  void handle_message(
      time_point recv_ts, long msgcnt, std::size_t msgoffset,
      message_type const& msg) {
    JB_LOG(trace) << msgcnt << ":" << msgoffset << " " << msg;
  }

  void handle_message(
      time_point recv_ts, long msgcnt, std::size_t msgoffset,
      jb::itch5::stock_directory_message const& msg) {
    JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
    books_.emplace(msg.stock, order_book());
  }

  void handle_message(
      time_point recv_ts, long msgcnt, std::size_t msgoffset,
      jb::itch5::add_order_message const& msg) {
    JB_LOG(trace) << " " << msgcnt << ":" << msgoffset << " " << msg;
    orders_.emplace(
        msg.order_reference_number, order_data{
          msg.stock, msg.buy_sell_indicator, msg.price, msg.shares});
    auto i = books_.find(msg.stock);
    if (i == books_.end()) {
      auto p = books_.emplace(msg.stock, order_book());
      i = p.first;
    }
    i->second.handle_add(msg.buy_sell_indicator, msg.price, msg.shares);
  }

  void handle_message(
      time_point recv_ts, long msgcnt, std::size_t msgoffset,
      jb::itch5::system_event_message const& msg) {
    JB_LOG(info) << " " << msgcnt << ":" << msgoffset << " " << msg;
  }

  void handle_unknown(
      time_point recv_ts, long msgcnt, std::size_t msgoffset,
      char const* msgbuf, std::size_t msglen) {
    JB_LOG(error) << "Unknown message type '" << msgbuf[0] << "'"
                  << " in msgcnt=" << msgcnt << ", msgoffset=" << msgoffset;
  }

  struct order_data {
    stock_t stock;
    buy_sell_indicator_t side;
    price4_t px;
    int qty;
  };

 private:
  typedef std::unordered_map<std::uint64_t, order_data> orders_by_id;
  orders_by_id orders_;

  typedef std::unordered_map<
    stock_t, order_book, boost::hash<stock_t>> books_by_security;
  books_by_security books_;
};

} // namespace itch5
} // namespace jb


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

  jb::itch5::inside_handler handler;
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
