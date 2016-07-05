#include <jb/itch5/message_header.hpp>
#include <jb/itch5/testing_data.hpp>
#include <jb/itch5/short_string_field.hpp>
#include <jb/assert_throw.hpp>

#include <chrono>
#include <list>
#include <thread>

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/test/unit_test.hpp>
#include <skye/mock_function.hpp>
#include <skye/mock_template_function.hpp>

struct mock_socket {
  skye::mock_function<void(int,std::vector<char>)> calls;

  template<typename ConstBufferSequence>
  void send(ConstBufferSequence const& buffers) {
  }
};

namespace jb {
namespace itch5 {

/**
 * Send a sequence of raw ITCH-5.x messages as MoldUDP packets, trying
 * to match the original time interval between messages.
 *
 * The MoldUDP protocol (see links below) allows transmission of
 * ITCH-5.x messages over UDP.  Multiple ITCH-5.x messages are packed
 * into a single MoldUDP packet, which includes enough information to
 * request retransmissions if needed.
 *
 * This class receives a stream of raw ITCH-5.x messages and creates a
 * stream of MoldUDP packets.  It examines the original timestamps of
 * the raw ITCH-5.x messages to pace the outgoing stream.  When the
 * original messages are sufficiently close in time they are assembled
 * into a single large packet.  If the messages are separated in time
 * the class blocks until enough wall-clock time has elapsed.
 *
 * @tparam clock_type a dependency injection point to make this class
 * testable.  Normally the class is simply used with a
 * std::chrono::steady_clock.  Under test, it is convenient to be able
 * to modify the results of the clock_type::now() function to exercise
 * multiple scenarios.
 *
 * References:
 *   https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/moldudp.pdf
 */
template<typename clock_type = std::chrono::steady_clock>
class mold_udp_pacer {
 public:
  //@{
  /**
   * @name Type traits
   */
  /// The time point (specific time with respect to some epoch) type.
  typedef typename clock_type::time_point time_point;

  /// The duration (the difference between two time points) type.
  typedef typename clock_type::duration duration;

  /**
   * The type used to represent session ids.
   *
   * The MoldUDP protocol uses a 10-character identifier for the
   * session id, different streams can be distinguished using this
   * field in the protocol.
   */
  typedef jb::itch5::short_string_field<10> session_id_type;
  //@}

  // TODO() all these parameters should be configurable ...
  /**
   * Initialize a MoldUDP pacer object.
   */
  mold_udp_pacer(session_id_type const& session_id = session_id_type())
      : last_send_()
      , max_delay_(std::chrono::microseconds(1))
      , mtu_(512)
      , full_message_(rawbuf, rawbufsize)
      , message_size_(header_size)
      , first_message_(0)
      , message_count_(0)
  {}

  /**
   * Process a raw ITCH-5.x message.
   *
   * @param ts The wall-clock when the message was received, as
   * defined by @a clock_type
   */
  template<typename message_sink_type, typename sleep_functor_type>
  void handle_message(
      time_point ts, long msgcnt, std::size_t msgoffset,
      char const* msgbuf, std::size_t msglen,
      message_sink_type& sink,
      sleep_functor_type sleeper = std::this_thread::sleep_for) {
    message_header msghdr = jb::itch5::decoder<false,message_header>::r(
        msglen, msgbuf, 0);
    
    // how long since the last send() call...
    auto elapsed = msghdr.timestamp.ts - last_send_.ts;

    if (elapsed <= max_delay_) {
      // ... save the message to send later, potentially flushing if
      // the queue is big enough ...
      coalesce(ts, msgcnt, msgoffset, msgbuf, msglen, sink);
      return;
    }
    // ... flush whatever is in the queue ...
    flush(sink);
    // ... until the timer has expired ...
    sleeper(elapsed);
    // ... send the message immediately ...
    coalesce(ts, msgcnt, msgoffset, msgbuf, msglen, sink);
  }

  template<typename message_sink_type>
  void flush(message_sink_type& sink) {
    if (message_count_ == 0) {
      return;
    }
    flush_impl(sink);
  }

  template<typename message_sink_type>
  void heartbeat(message_sink_type& sink) {
    flush_impl(sink);
  }

 private:
  template<typename message_sink_type>
  void coalesce(
      time_point ts, long msgcnt, std::size_t msgoffset,
      char const* msgbuf, std::size_t msglen,
      message_sink_type& sink) {
    JB_ASSERT_THROW(msglen < mtu_ - header_size);
    JB_ASSERT_THROW(msglen < (1<<16));
    if (msglen + 2 + message_size_ > mtu_) {
      flush(sink);
    }
    first_message_ = msgcnt;
    
    boost::asio::mutable_buffer block_header = full_message_ + message_size_;
    std::uint8_t* raw = boost::asio::buffer_cast<std::uint8_t*>(block_header);
    raw[0] = msglen & 0xff;
    raw[1] = (msglen & 0xff00) >> 8;
    boost::asio::mutable_buffer block_payload =
        full_message_ + message_size_ + 2;
    boost::asio::buffer_copy(
        block_payload, boost::asio::buffer(msgbuf, msglen));
    message_size_ += msglen + 2;
    
    message_count_++;
  }

  void create_header() {
    std::uint8_t* seqno = boost::asio::buffer_cast<std::uint8_t*>(
        full_message_ + sequence_number_offset);
    seqno[0] = (first_message_ & 0xff);
    seqno[1] = (first_message_ & 0xff00) >> 8;
    seqno[2] = (first_message_ & 0xff0000) >> 16;
    seqno[3] = (first_message_ & 0xff000000) >> 24;
    std::uint8_t* msgcnt = boost::asio::buffer_cast<std::uint8_t*>(
        full_message_ + message_count_offset);
    msgcnt[0] = (message_count_ & 0xff);
    msgcnt[1] = (message_count_ & 0xff00) >> 8;
  }

  void flushed() {
    first_message_ = first_message_ + message_count_;
    message_count_ = 0;
    message_size_ = 0;
  }    

  template<typename message_sink_type>
  void flush_impl(message_sink_type& sink) {
    create_header();
    sink.send(boost::asio::buffer(full_message_, message_size_));
    flushed();
  }
  
  
 private:
  jb::itch5::timestamp last_send_;
  duration max_delay_;
  int mtu_;

  static constexpr std::size_t rawbufsize = 65536;
  static constexpr std::size_t header_size = 16;
  static constexpr std::size_t sequence_number_offset = 10;
  static constexpr std::size_t message_count_offset = 14;
  char rawbuf[rawbufsize] = {0};

  boost::asio::mutable_buffer full_message_;
  std::size_t message_size_;

  std::uint32_t first_message_;
  std::uint16_t message_count_;
};

} // namespace itch5
} // namespace jb

struct mock_clock : public std::chrono::steady_clock {
  static skye::mock_function<time_point()> now;
};

skye::mock_function<mock_clock::time_point()> mock_clock::now;

/**
 * @test Verify that the packet pacer works as expected.
 */
BOOST_AUTO_TEST_CASE(packet_pacer_basic) {

  skye::mock_function<void(mock_clock::duration const&)> mock_sleep;
  mock_socket socket;
  jb::itch5::mold_udp_pacer<mock_clock> p;

  std::size_t const msglen = 24;
  char msgbuf[msglen] = {0};

  mock_clock::now.action([]() {
      static int ts = 0;
      return mock_clock::time_point(std::chrono::microseconds(++ts));
    });

  p.handle_message(
      mock_clock::time_point(), 0, 0, msgbuf, msglen, socket, mock_sleep);
}
