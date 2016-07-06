#include <jb/itch5/mold_udp_pacer.hpp>
#include <jb/itch5/testing_data.hpp>

#include <chrono>
#include <list>
#include <thread>

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/test/unit_test.hpp>
#include <skye/mock_function.hpp>

/**
 * A simple class to mock Boost.ASIO sockets.
 *
 * Boost.ASIO sockets use template member functions in their
 * implementation, which makes them a pain to mock.  This simply
 * captures all the results in a vector of vectors that the user can
 * analyze later.
 */
struct mock_socket {
  std::vector<std::vector<char>> packets;

  template<typename const_buffer_sequence>
  void send(const_buffer_sequence const& buffers) {
    std::vector<char> packet(boost::asio::buffer_size(buffers));
    buffer_copy(boost::asio::buffer(packet), buffers);
    packets.push_back(packet);
  }
};

struct mock_clock : public std::chrono::steady_clock {
  static skye::mock_function<time_point()> now;
};

skye::mock_function<mock_clock::time_point()> mock_clock::now;

/**
 * @test Verify that the packet pacer works as expected for a simple
 * stream of messages.
 */
BOOST_AUTO_TEST_CASE(packet_pacer_basic) {

  skye::mock_function<void(mock_clock::duration const&)> mock_sleep;
  mock_socket socket;
  jb::itch5::mold_udp_pacer<mock_clock> p(
      jb::itch5::mold_udp_pacer_config().maximum_delay_microseconds(5));

  mock_clock::now.action([]() {
      static int ts = 0;
      return mock_clock::time_point(std::chrono::microseconds(++ts));
    });

  /// Send 3 messages every 10 usecs, of different sizes and types
  auto m1 = jb::itch5::testing::create_message(
      'A', jb::itch5::timestamp{std::chrono::microseconds(5)}, 100);
  p.handle_message(
      mock_clock::now(), jb::itch5::unknown_message(
          0, 0, m1.size(), &m1[0]), socket, mock_sleep);

  auto m2 = jb::itch5::testing::create_message(
      'B', jb::itch5::timestamp{std::chrono::microseconds(15)}, 90);
  p.handle_message(
      mock_clock::now(), jb::itch5::unknown_message(
          0, 0, m2.size(), &m2[0]), socket, mock_sleep);

  auto m3 = jb::itch5::testing::create_message(
      'A', jb::itch5::timestamp{std::chrono::microseconds(25)}, 80);
  p.handle_message(
      mock_clock::now(), jb::itch5::unknown_message(
          0, 0, m3.size(), &m3[0]), socket, mock_sleep);

  p.flush(socket);

  auto hdrsize = jb::itch5::mold_udp_protocol::header_size;
  BOOST_REQUIRE_GE(socket.packets.size(), 3);
  BOOST_CHECK_EQUAL(socket.packets.size(), 3);
  BOOST_CHECK_EQUAL(100 + 2 + hdrsize, socket.packets.at(0).size());
  BOOST_CHECK_EQUAL(90 + 2 + hdrsize, socket.packets.at(1).size());
  BOOST_CHECK_EQUAL(80 + 2 + hdrsize, socket.packets.at(2).size());
}
