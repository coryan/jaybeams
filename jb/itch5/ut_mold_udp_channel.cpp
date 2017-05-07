#include <jb/itch5/make_socket_udp_recv.hpp>
#include <jb/itch5/mold_udp_channel.hpp>
#include <jb/itch5/mold_udp_protocol_constants.hpp>
#include <jb/itch5/testing/data.hpp>
#include <jb/itch5/timestamp.hpp>

#include <skye/mock_function.hpp>
#include <boost/test/unit_test.hpp>

/**
 * Helper types and functions to test jb::itch5::mold_udp_channel
 */
namespace {
std::vector<char>
create_mold_udp_packet(std::uint64_t sequence_number, int message_count) {
  std::size_t const max_packet_size = 1 << 16;
  char packet[max_packet_size];

  jb::itch5::encoder<true, std::uint64_t>::w(
      max_packet_size, packet,
      jb::itch5::mold_udp_protocol::sequence_number_offset, sequence_number);
  jb::itch5::encoder<true, std::uint16_t>::w(
      max_packet_size, packet, jb::itch5::mold_udp_protocol::block_count_offset,
      message_count);

  std::size_t packet_size = jb::itch5::mold_udp_protocol::header_size;
  boost::asio::mutable_buffer dst(packet, packet_size);

  char msg_type = 'A';
  int ts = 5;
  for (int i = 0; i != message_count; ++i) {
    auto message = jb::itch5::testing::create_message(
        msg_type, jb::itch5::timestamp{std::chrono::microseconds(ts)}, 64);
    ts += 5;
    msg_type++;
    jb::itch5::encoder<true, std::uint16_t>::w(
        max_packet_size, packet, packet_size, message.size());
    packet_size += 2;
    boost::asio::buffer_copy(dst + packet_size, boost::asio::buffer(message));
    packet_size += message.size();
  }
  return std::vector<char>(packet, packet + packet_size);
}

/**
 * Pick a localhost address that is valid on the testing host.
 *
 * In some testing hosts (notably travis-ci.org) the host does not
 * support IPv6 addresses.  We need to determine, at run-time, a valid
 * address to test the code.  A separate test
 * (ut_make_socket_upd_recv) validates that the library works with any
 * address and fails gracefully.  In this test we just want to move
 * forward.
 *
 * @param io the Boost.ASIO io service
 * @returns a string with the valid localhost address, typically ::1,
 * but can be 127.0.0.1 if IPv6 is not functional
 * @throws std::exception if no valid localhost address is found
 */
std::string select_localhost_address(boost::asio::io_service& io) {
  for (auto const& addr : {"::1", "127.0.0.1"}) {
    try {
      BOOST_TEST_CHECKPOINT("Checking " << addr << " as the localhost address");
      auto socket = jb::itch5::make_socket_udp_recv(io, addr, 40000, "");
      return addr;
    } catch (...) {
    }
  }
  BOOST_TEST_MESSAGE("No valid localhost address found, aborting");
  throw std::runtime_error("Cannot find valid IPv6 or IPv4 address");
}
} // anonymous namespace

namespace jb {
namespace itch5 {
/**
 * Break encapsulation in jb::itch5::mold_udp_channel for testing purposes.
 */
struct mold_udp_channel_tester {
  static void call_with_empty_packet(mold_udp_channel& tested) {
    tested.handle_received(boost::system::error_code(), 0);
  }
  static void call_with_error_code(mold_udp_channel& tested) {
    tested.handle_received(
        boost::asio::error::make_error_code(boost::asio::error::network_down),
        16);
  }
};

} // namespace itch5
} // namespace jb

/**
 * @test Verify that jb::itch5::mold_udp_channel works.
 */
BOOST_AUTO_TEST_CASE(itch5_mold_udp_channel_basic) {
  skye::mock_function<void(
      std::chrono::steady_clock::time_point, std::uint64_t, std::size_t,
      std::string, std::size_t)>
      handler;
  auto adapter = [&handler](
      std::chrono::steady_clock::time_point ts, std::uint64_t seqno,
      std::size_t offset, char const* msg, std::size_t msgsize) {
    handler(ts, seqno, offset, std::string(msg, msgsize), msgsize);
  };

  using boost::asio::ip::udp;

  boost::asio::io_service io;
  auto local = select_localhost_address(io);
  BOOST_TEST_MESSAGE("Running test on " << local);
  jb::itch5::mold_udp_channel channel(io, adapter, local, 50000, "");

  udp::resolver resolver(io);
  udp::endpoint send_to;
  udp::endpoint send_from;
  auto d_address = boost::asio::ip::address::from_string(local);
  if (d_address.is_v6()) {
    BOOST_TEST_CHECKPOINT(
        "Resolving dst/src IPv6 addresses for " << local << ":50000");
    send_to = *resolver.resolve({udp::v6(), local, "50000"});
    send_from = udp::endpoint(udp::v6(), 0);
  } else {
    BOOST_TEST_CHECKPOINT(
        "Resolving dst/src IPv4 addresses for " << local << ":50000");
    send_to = *resolver.resolve({udp::v4(), local, "50000"});
    send_from = udp::endpoint(udp::v4(), 0);
  }
  udp::socket socket(io, send_from);

  auto packet = create_mold_udp_packet(0, 3);
  socket.send_to(boost::asio::buffer(packet), send_to);
  io.run_one();
  handler.check_called().exactly(3);

  packet = create_mold_udp_packet(0, 3);
  socket.send_to(boost::asio::buffer(packet), send_to);
  io.run_one();
  handler.check_called().exactly(6);

  packet = create_mold_udp_packet(9, 2);
  socket.send_to(boost::asio::buffer(packet), send_to);
  io.run_one();
  handler.check_called().exactly(8);

  packet = create_mold_udp_packet(12, 1);
  socket.send_to(boost::asio::buffer(packet), send_to);
  io.run_one();
  handler.check_called().exactly(9);

  packet = create_mold_udp_packet(13, 0);
  socket.send_to(boost::asio::buffer(packet), send_to);
  io.run_one();
  handler.check_called().exactly(9);
}

/**
 * @test Comlete code coverage for jb::itch5::mold_udp_channel.
 */
BOOST_AUTO_TEST_CASE(itch5_mold_udp_channel_coverage) {
  auto adapter =
      [](std::chrono::steady_clock::time_point ts, std::uint64_t seqno,
         std::size_t offset, char const* msg, std::size_t msgsize) {};

  using boost::asio::ip::udp;

  boost::asio::io_service io;
  auto local = select_localhost_address(io);
  jb::itch5::mold_udp_channel channel(io, adapter, local, 50000, "");

  jb::itch5::mold_udp_channel_tester::call_with_empty_packet(channel);
  jb::itch5::mold_udp_channel_tester::call_with_error_code(channel);
}
