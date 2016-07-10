#ifndef jb_itch5_mold_udp_pacer_hpp
#define jb_itch5_mold_udp_pacer_hpp

#include <jb/itch5/encoder.hpp>
#include <jb/itch5/message_header.hpp>
#include <jb/itch5/mold_udp_pacer_config.hpp>
#include <jb/itch5/mold_udp_protocol_constants.hpp>
#include <jb/itch5/short_string_field.hpp>
#include <jb/itch5/unknown_message.hpp>
#include <jb/assert_throw.hpp>

#include <boost/asio/buffer.hpp>

namespace jb {
namespace itch5 {

/**
 * Send a sequence of raw ITCH-5.x messages as MoldUDP64 packets, trying
 * to match the original time interval between messages.
 *
 * The MoldUDP64 protocol (see link below) allows transmission of
 * ITCH-5.x messages over UDP.  Multiple ITCH-5.x messages are packed
 * into a single MoldUDP64 packet, which includes enough information to
 * request retransmissions if needed.
 *
 * This class receives a stream of raw ITCH-5.x messages and creates a
 * stream of MoldUDP64 packets.  It examines the original timestamps of
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
 *   http://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/moldudp64.pdf
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

  /// The configuration file
  typedef mold_udp_pacer_config config;

  /**
   * The type used to represent session ids.
   *
   * The MoldUDP64 protocol uses a 10-character identifier for the
   * session id, different streams can be distinguished using this
   * field in the protocol.
   */
  typedef jb::itch5::short_string_field<
    mold_udp_protocol::session_id_size> session_id_type;
  //@}

  /**
   * Initialize a MoldUDP pacer object.
   */
  mold_udp_pacer(
      config const& cfg = config(),
      session_id_type const& session_id = session_id_type())
      : last_send_{std::chrono::microseconds(0)}
      , max_delay_(std::chrono::microseconds(cfg.maximum_delay_microseconds()))
      , mtu_(cfg.maximum_transmission_unit())
      , packet_(rawbuf, rawbufsize)
      , packet_size_(mold_udp_protocol::header_size)
      , first_block_(0)
      , first_block_ts_{std::chrono::microseconds(0)}
      , block_count_(0)
  {}

  /**
   * Process a raw ITCH-5.x message.
   *
   * @param ts the wall-clock when the message was received, as
   * defined by @a clock_type
   * @param msg the message received, the timestamp in the message is
   * used to pace the outgoing MoldUDP64 packets
   * @param sink a functor to send the MoldUDP64 packets
   * @param sleeper a functor to sleep and effectively pace the
   * messages
   *
   * @tparam message_sink_type the type of the @a sink functor.  The
   * signature must be compatible with void(auto buffers) where
   * buffers meets the requirements of a Boost.Asio ConstBufferSequence.
   * @tparam sleep_functor_type the type of the sleeper function, the
   * signature must be compatible with void(clock_type::duration const&)
   */
  template<typename message_sink_type, typename sleep_functor_type>
  void handle_message(
      time_point ts, unknown_message const& msg,
      message_sink_type& sink, sleep_functor_type& sleeper) {
    message_header msghdr = msg.decode_header<false>();
    
    // how long since the last send() call...
    if (msg.count() == 0) {
      // ... on the first message initialize the timestamp, otherwise
      // we would likely flush the first message always ...
      last_send_ = msghdr.timestamp;
    }
    auto elapsed = msghdr.timestamp.ts - last_send_.ts;
    if (elapsed < max_delay_) {
      // ... save the message to send later, potentially flushing if
      // the queue is big enough ...
      coalesce(ts, msg, msghdr.timestamp, sink);
      return;
    }
    // ... flush whatever is in the queue ...
    flush(msghdr.timestamp, sink);
    // ... until the timer has expired ...
    sleeper(elapsed);
    // ... send the message immediately ...
    coalesce(ts, msg, msghdr.timestamp, sink);
  }

  /**
   * Flush the current messages, if any
   *
   * @param ts the wall clock time when the message was received
   * @param sink the destination for the MoldUDP64 packets
   *
   * @tparam message_sink_type please see handle_message() for details
   */
  template<typename message_sink_type>
  void flush(timestamp ts, message_sink_type& sink) {
    if (block_count_ == 0) {
      return;
    }
    flush_impl(ts, sink);
  }

  /**
   * Send a heartbeat packet.
   *
   * If there are any pending messages those messages are flushed and
   * the resulting packet constitutes the heartbeat.
   *
   * @param sink the destination for the MoldUDP64 packets
   *
   * @tparam message_sink_type please see handle_message() for details
   */
  template<typename message_sink_type>
  void heartbeat(message_sink_type& sink) {
    flush_impl(first_block_ts_, sink);
  }

 private:
  /**
   * Add another message to the current queue, flushing first if
   * necessary.
   *
   * @param recv_ts the timestamp when the message was received
   * @param msg the message contents and location
   * @param ts the timestamp when the last message was sent
   * @param sink the destination for the MoldUDP64 packets
   */
  template<typename message_sink_type>
  void coalesce(
      time_point recv_ts, unknown_message const& msg,
      timestamp ts, message_sink_type& sink) {
    // Make sure the message is small enough to be represented in a
    // single MoldUDP64 block ...
    JB_ASSERT_THROW(msg.len() < (1<<16));
    // ... make sure the message is small enough to fit in a single
    // MoldUDP64 packet given the current MTU ...
    JB_ASSERT_THROW(msg.len() < mtu_ - mold_udp_protocol::header_size - 2);

    // ... if the packet is too full to accept the current message,
    // flush first ...
    if (packet_full(msg.len())) {
      flush(ts, sink);
    }
    if (block_count_ == 0) {
      first_block_ = msg.count();
      first_block_ts_ = ts;
    }      
    // ... append the message as a new block in the MoldUDP packet,
    // first update the block header ...
    boost::asio::mutable_buffer block_header = packet_ + packet_size_;
    std::uint8_t* raw = boost::asio::buffer_cast<std::uint8_t*>(block_header);
    raw[0] = msg.len() & 0xff;
    raw[1] = (msg.len() & 0xff00) >> 8;
    // ... the copy the message into the block payload ...
    boost::asio::mutable_buffer block_payload = packet_ + packet_size_ + 2;
    boost::asio::buffer_copy(
        block_payload, boost::asio::buffer(msg.buf(), msg.len()));
    packet_size_ += msg.len() + 2;

    // ... and update the number of blocks ...
    block_count_++;
  }

  /// Fill up the header for the MoldUDP64 packet
  void fillup_header_fields() {
    // ... we assume that the session was initialized in the
    // constructor, and simply reuse that portion of the packet over
    // and over.
    // ... write down the sequence number field of the packet header,
    // this is the number of the first block ...
    auto seqno = packet_ + mold_udp_protocol::sequence_number_offset;
    encoder<true,std::uint64_t>::w(
        boost::asio::buffer_size(seqno),
        boost::asio::buffer_cast<void*>(seqno), 0, first_block_);
    // ... then write down the block field ...
    auto blkcnt = packet_ + mold_udp_protocol::block_count_offset;
    encoder<true,std::uint16_t>::w(
        boost::asio::buffer_size(blkcnt),
        boost::asio::buffer_cast<void*>(blkcnt), 0, block_count_);
  }

  /**
   * Implement the flush() and hearbeat() member functions.
   */
  template<typename message_sink_type>
  void flush_impl(timestamp ts, message_sink_type& sink) {
    fillup_header_fields();
    sink(boost::asio::buffer(packet_, packet_size_));
    last_send_ = ts;
    first_block_ = first_block_ + block_count_;
    block_count_ = 0;
    packet_size_ = mold_udp_protocol::header_size;
  }

  /**
   * Return true if the packet is too full to accept a new block of
   * size @a block_size.
   *
   * @param block_size the size of the next block that we intend to
   * add to the packet
   */
  bool packet_full(std::uint16_t block_size) const {
    if (block_size + 2 + packet_size_ >= std::size_t(mtu_)) {
      return true;
    }
    return block_count_ == std::numeric_limits<std::uint16_t>::max();
  }
  
 private:
  jb::itch5::timestamp last_send_;
  duration max_delay_;
  int mtu_;

  // Use a simple raw buffer to hold the packet, this is good enough
  // because MoldUDP64 can only operate on UDP packets, which never
  // exceed 64KiB.
  static constexpr std::size_t rawbufsize = 65536;
  char rawbuf[rawbufsize] = {0};

  boost::asio::mutable_buffer packet_;
  std::size_t packet_size_;

  std::uint32_t first_block_;
  timestamp first_block_ts_;
  std::uint16_t block_count_;
};

} // namespace itch5
} // namespace jb

#endif // jb_itch5_mold_udp_pacer_hpp
