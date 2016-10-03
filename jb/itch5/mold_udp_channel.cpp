#include <jb/itch5/mold_udp_channel.hpp>

#include <jb/itch5/base_decoders.hpp>
#include <jb/itch5/mold_udp_protocol_constants.hpp>
#include <jb/log.hpp>

#include <boost/asio/ip/multicast.hpp>

namespace jb {
namespace itch5 {

mold_udp_channel::mold_udp_channel(
    buffer_handler handler,
    boost::asio::io_service& io,
    std::string const& listen_address,
    int multicast_port,
    std::string const& multicast_group)
    : handler_(handler)
    , socket_(io)
    , expected_sequence_number_(0)
    , message_offset_(0) {
  auto group_address = boost::asio::ip::address::from_string(multicast_group);

  boost::asio::ip::address address;

  // Automatically configure the best listening address ...
  if (listen_address != "") {
    // ... the user speficied a listening address, use that ...
    address = boost::asio::ip::address::from_string(listen_address);
  } else if (not group_address.is_multicast()) {
    address = group_address;
  } else {
    // ... pick a default based on the protocol for the listening
    // group ...
    if (group_address.is_v6()) {
      address = boost::asio::ip::address_v6();
    } else {
      address = boost::asio::ip::address_v4();
    }
  }
  
  boost::asio::ip::udp::endpoint endpoint(address, multicast_port);
  boost::asio::ip::udp::socket socket(io);
  socket_.open(endpoint.protocol());
  socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));

  JB_LOG(info) << "Requested endpoint=" << endpoint;
  socket_.bind(endpoint);
  JB_LOG(info) << " .. bound to endpoint=" << socket_.local_endpoint();
  if (group_address.is_multicast()) {
    socket_.set_option(
        boost::asio::ip::multicast::join_group(group_address));
    socket_.set_option(boost::asio::ip::multicast::enable_loopback(true));
    JB_LOG(info) << " .. joining multicast group=" << group_address;
  }

  restart_async_receive_from();
}

void mold_udp_channel::restart_async_receive_from() {
  socket_.async_receive_from(
      boost::asio::buffer(buffer_, buflen), sender_endpoint_,
      [this](boost::system::error_code const& ec, size_t bytes_received) {
        handle_received(ec, bytes_received);
      });
}

void mold_udp_channel::handle_received(
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
    std::size_t offset = jb::itch5::mold_udp_protocol::header_size;
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
      offset += message_size;
    }
    // ... since we are not dealing with gaps, or message reordering
    // just reset the next expected number ...
    expected_sequence_number_ = sequence_number;
  }
  restart_async_receive_from();
}

} // namespace itch5
} // namespace jb
