/**
 * @file
 *
 * This program reads a raw ITCH-5.0 file and prints out the trade
 * messages into an ASCII (though potentially compressed) file.
 */
#include <jb/itch5/process_iostream.hpp>
#include <jb/fileio.hpp>
#include <jb/log.hpp>

#include <stdexcept>

/**
 * Define types and functions used in this program.
 */
namespace {

/// Configuration parameters for itch5inside
class config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  void validate() const override;

  jb::config_attribute<config, std::string> input_file;
  jb::config_attribute<config, std::string> output_file;
  jb::config_attribute<config, jb::log::config> log;
};

/**
 * Filter the jb::itch5::trade_message and print them to a std::ostream.
 */
class trades_handler {
public:
  //@{
  /**
   * @name Type traits
   */
  /// Define the clock used to measure processing delays
  typedef std::chrono::steady_clock clock_type;

  /// A convenience typedef for clock_type::time_point
  typedef typename clock_type::time_point time_point;
  //@}

  /// Constructor, capture the output stream
  explicit trades_handler(std::ostream& out)
      : out_(out) {
  }

  /**
   * Handle a trade message, print it out to the output stream.
   *
   * @param header the header of the raw ITCH-5.0 message
   * @param update a representation of the update just applied to the
   * book
   * @param msg the trade message
   */
  void handle_message(
      time_point recvts, long msgcnt, std::size_t msgoffset,
      jb::itch5::trade_message const& msg);

  /**
   * Ignore all other message types.
   *
   * We are only interested in a handful of message types, anything
   * else is captured by this template function and ignored.
   *
   * @tparam message_type the type of message to ignore
   */
  template <typename message_type>
  void handle_message(time_point, long, std::size_t, message_type const&) {
  }

  /**
   * Log any unknown message types.
   *
   * @param recvts the timestamp when the message was received
   * @param msg the unknown message location and contents
   */
  void handle_unknown(time_point recvts, jb::itch5::unknown_message const& msg);

  /// Return the current timestamp for delay measurements
  time_point now() const {
    return std::chrono::steady_clock::now();
  }

private:
  std::ostream& out_;
};

} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.load_overrides(argc, argv, std::string("itch5trades.yaml"), "JB_ROOT");
  jb::log::init(cfg.log());

  boost::iostreams::filtering_istream in;
  jb::open_input_file(in, cfg.input_file());

  boost::iostreams::filtering_ostream out;
  jb::open_output_file(out, cfg.output_file());

  trades_handler handler(out);
  jb::itch5::process_iostream(in, handler);

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

config::config()
    : input_file(
          desc("input-file").help("An input file with ITCH-5.0 messages."),
          this)
    , output_file(
          desc("output-file")
              .help("The name of the file where to store the inside data."
                    "  Files ending in .gz are automatically compressed."),
          this, "stdout")
    , log(desc("log", "logging"), this) {
}

void config::validate() const {
  if (input_file() == "") {
    throw jb::usage(
        "Missing input-file setting."
        "  You must specify an input file.",
        1);
  }
  if (output_file() == "") {
    throw jb::usage(
        "Missing output-file setting."
        "  You must specify an output file.",
        1);
  }
  log().validate();
}

void trades_handler::handle_message(
    time_point, long, std::size_t, jb::itch5::trade_message const& msg) {
  out_ << msg.header.timestamp.ts.count() << " " << msg.order_reference_number
       << " " << static_cast<char>(msg.buy_sell_indicator.as_int()) << " "
       << msg.shares << " " << msg.stock << " " << msg.price << " "
       << msg.match_number << "\n";
}

void trades_handler::handle_unknown(
    time_point recvts, jb::itch5::unknown_message const& msg) {
  char msgtype = *static_cast<char const*>(msg.buf());
  JB_LOG(error) << "Unknown message type '" << msgtype << "'(" << int(msgtype)
                << ") in msgcnt=" << msg.count()
                << ", msgoffset=" << msg.offset();
}

} // anonymous namespace
