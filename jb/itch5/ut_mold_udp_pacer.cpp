#include <jb/itch5/mold_udp_pacer.hpp>
#include <jb/itch5/testing_data.hpp>

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
 * @test Verify that jb::itch5::mold_udp_pacer works as expected for a
 * simple stream of messages.
 */
BOOST_AUTO_TEST_CASE(itch5_mold_udp_pacer_basic) {
  skye::mock_function<void(mock_clock::duration const&)> mock_sleep;
  mock_socket socket;
  mock_clock::now.action([]() {
      static int ts = 0;
      return mock_clock::time_point(std::chrono::microseconds(++ts));
    });

  jb::itch5::mold_udp_pacer<mock_clock> p(
      jb::itch5::mold_udp_pacer_config().maximum_delay_microseconds(5));
  
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
          1, 0, m2.size(), &m2[0]), socket, mock_sleep);

  auto m3 = jb::itch5::testing::create_message(
      'A', jb::itch5::timestamp{std::chrono::microseconds(25)}, 80);
  p.handle_message(
      mock_clock::now(), jb::itch5::unknown_message(
          2, 0, m3.size(), &m3[0]), socket, mock_sleep);

  p.heartbeat(socket);

  auto hdrsize = jb::itch5::mold_udp_protocol::header_size;
  BOOST_REQUIRE_EQUAL(socket.packets.size(), 3);
  BOOST_CHECK_EQUAL(100 + 2 + hdrsize, socket.packets.at(0).size());
  BOOST_CHECK_EQUAL(90 + 2 + hdrsize, socket.packets.at(1).size());
  BOOST_CHECK_EQUAL(80 + 2 + hdrsize, socket.packets.at(2).size());
}

/**
 * @test Verify that multiple back-to-back messages are grouped into a
 * single packet.
 */
BOOST_AUTO_TEST_CASE(itch5_mold_udp_pacer_coalesce) {
  // create all the mock objects ...
  skye::mock_function<void(mock_clock::duration const&)> mock_sleep;
  mock_clock::now.action([]() {
      static int ts = 0;
      return mock_clock::time_point(std::chrono::microseconds(++ts));
    });
  mock_socket socket;

  // ... create a pacer that commits up to 1024 bytes and blocks for
  // up to a second ...
  jb::itch5::mold_udp_pacer<mock_clock> p(
      jb::itch5::mold_udp_pacer_config()
      .maximum_delay_microseconds(1000000)
      .maximum_transmission_unit(1024));

  using namespace std::chrono;
  // simulate 3 messages every 10 usecs, of different sizes and types ...
  int msgcnt = 0;
  auto m1 = jb::itch5::testing::create_message(
      'A', jb::itch5::timestamp{std::chrono::microseconds(5)}, 100);
  p.handle_message(
      mock_clock::now(), jb::itch5::unknown_message(
          msgcnt++, 0, m1.size(), &m1[0]), socket, mock_sleep);

  auto m2 = jb::itch5::testing::create_message(
      'B', jb::itch5::timestamp{std::chrono::microseconds(15)}, 90);
  p.handle_message(
      mock_clock::now(), jb::itch5::unknown_message(
          msgcnt++, 0, m2.size(), &m2[0]), socket, mock_sleep);

  auto m3 = jb::itch5::testing::create_message(
      'A', jb::itch5::timestamp{std::chrono::microseconds(25)}, 80);
  p.handle_message(
      mock_clock::now(), jb::itch5::unknown_message(
          msgcnt++, 0, m3.size(), &m3[0]), socket, mock_sleep);

  // ... we expect that no messages have been sent so far ...
  BOOST_CHECK_EQUAL(socket.packets.size(), 0);

  // ... we force a flush ...
  p.heartbeat(socket);

  auto hdrsize = jb::itch5::mold_udp_protocol::header_size;
  // ... we should receive a single packet with all 3 messages ...
  BOOST_REQUIRE_EQUAL(socket.packets.size(), 1);
  BOOST_CHECK_EQUAL(
      hdrsize + 100 + 2 + 90 + 2 + 80 + 2, socket.packets.at(0).size());
}

/**
 * @test Verify that multiple back-to-back messages are flushed if the
 * packet is about to get full...
 */
BOOST_AUTO_TEST_CASE(itch5_mold_udp_pacer_flush_full) {
  // create all the mock objects ...
  skye::mock_function<void(mock_clock::duration const&)> mock_sleep;
  mock_clock::now.action([]() {
      static int ts = 0;
      return mock_clock::time_point(std::chrono::microseconds(++ts));
    });
  mock_socket socket;

  // ... create a pacer that commits up to 220 bytes and blocks for
  // up to a second ...
  jb::itch5::mold_udp_pacer<mock_clock> p(
      jb::itch5::mold_udp_pacer_config()
      .maximum_delay_microseconds(1000000)
      .maximum_transmission_unit(220));

  using namespace std::chrono;
  // simulate 3 messages every 10 usecs, of different sizes and types ...
  int msgcnt = 0;
  auto m1 = jb::itch5::testing::create_message(
      'A', jb::itch5::timestamp{std::chrono::microseconds(5)}, 100);
  p.handle_message(
      mock_clock::now(), jb::itch5::unknown_message(
          msgcnt++, 0, m1.size(), &m1[0]), socket, mock_sleep);

  auto m2 = jb::itch5::testing::create_message(
      'B', jb::itch5::timestamp{std::chrono::microseconds(15)}, 90);
  p.handle_message(
      mock_clock::now(), jb::itch5::unknown_message(
          msgcnt++, 0, m2.size(), &m2[0]), socket, mock_sleep);

  // ... we expect that no messages have been sent so far ...
  BOOST_CHECK_EQUAL(socket.packets.size(), 0);

  auto m3 = jb::itch5::testing::create_message(
      'A', jb::itch5::timestamp{std::chrono::microseconds(25)}, 80);
  p.handle_message(
      mock_clock::now(), jb::itch5::unknown_message(
          msgcnt++, 0, m3.size(), &m3[0]), socket, mock_sleep);

  auto hdrsize = jb::itch5::mold_udp_protocol::header_size;
  // ... we should receive a single packet with the first 2 messages ...
  BOOST_REQUIRE_EQUAL(socket.packets.size(), 1);
  BOOST_CHECK_EQUAL(
      hdrsize + 100 + 2 + 90 + 2, socket.packets.at(0).size());

  // ... create a heartbeat, that should flush the last message ...
  p.heartbeat(socket);
  BOOST_REQUIRE_EQUAL(socket.packets.size(), 2);
  BOOST_CHECK_EQUAL(
      hdrsize + 80 + 2, socket.packets.at(1).size());
}

/**
 * @test Verify that multiple back-to-back messages are flushed if the
 * packet is about to get full...
 */
BOOST_AUTO_TEST_CASE(itch5_mold_udp_pacer_flush_timeout) {
  skye::mock_function<void(mock_clock::duration)> mock_sleep;
  mock_clock::now.action([]() {
      static int ts = 0;
      return mock_clock::time_point(std::chrono::microseconds(++ts));
    });
  mock_socket socket;

  // ... create a pacer that commits up to 1024 bytes and blocks for
  // up to a millisecond ...
  jb::itch5::mold_udp_pacer<mock_clock> p(
      jb::itch5::mold_udp_pacer_config()
      .maximum_delay_microseconds(1000)
      .maximum_transmission_unit(1024));

  using namespace std::chrono;
  // simulate 2 messages every 10 usecs, of different sizes and types ...
  int msgcnt = 0;
  auto m1 = jb::itch5::testing::create_message(
      'A', jb::itch5::timestamp{std::chrono::microseconds(5)}, 100);
  p.handle_message(
      mock_clock::now(), jb::itch5::unknown_message(
          msgcnt++, 0, m1.size(), &m1[0]), socket, mock_sleep);

  auto m2 = jb::itch5::testing::create_message(
      'B', jb::itch5::timestamp{std::chrono::microseconds(15)}, 90);
  p.handle_message(
      mock_clock::now(), jb::itch5::unknown_message(
          msgcnt++, 0, m2.size(), &m2[0]), socket, mock_sleep);

  // ... we expect that no messages have been sent so far ...
  BOOST_CHECK_EQUAL(socket.packets.size(), 0);

  // ... the next message is much later ...
  auto m3 = jb::itch5::testing::create_message(
      'A', jb::itch5::timestamp{std::chrono::microseconds(2025)}, 80);
  p.handle_message(
      mock_clock::now(), jb::itch5::unknown_message(
          msgcnt++, 0, m3.size(), &m3[0]), socket, mock_sleep);

  auto hdrsize = jb::itch5::mold_udp_protocol::header_size;
  // ... that should immediately flush the first two messages ...
  BOOST_REQUIRE_EQUAL(socket.packets.size(), 1);
  BOOST_CHECK_EQUAL(
      hdrsize + 100 + 2 + 90 + 2, socket.packets.at(0).size());

  // ... it should also create a call to sleep for 1025 - 15
  // microseconds ...
  mock_sleep.require_called().once();
  BOOST_CHECK_EQUAL(
      static_cast<mock_clock::duration>(std::get<0>(mock_sleep.at(0))).count(),
      mock_clock::duration(std::chrono::microseconds(2020)).count());
}

/**
 * @test Verify that flush() on an empty packet does not produce a
 * send() request.
 */
BOOST_AUTO_TEST_CASE(itch5_mold_udp_pacer_flush_on_empty) {
  skye::mock_function<void(mock_clock::duration)> mock_sleep;
  mock_clock::now.action([]() {
      static int ts = 0;
      return mock_clock::time_point(std::chrono::microseconds(++ts));
    });
  mock_socket socket;

  // ... create a pacer that commits up to 1024 bytes and blocks for
  // up to a millisecond ...
  jb::itch5::mold_udp_pacer<mock_clock> p(
      jb::itch5::mold_udp_pacer_config()
      .maximum_delay_microseconds(1000)
      .maximum_transmission_unit(1024));

  using namespace std::chrono;
  jb::itch5::timestamp ts{std::chrono::microseconds(5)};
  auto m = jb::itch5::testing::create_message('A', ts, 100);
  p.handle_message(
      mock_clock::now(), jb::itch5::unknown_message(
          0, 0, m.size(), &m[0]), socket, mock_sleep);

  // ... we expect that no messages have been sent so far ...
  BOOST_CHECK_EQUAL(socket.packets.size(), 0);

  // ... this flush() request should result in at least one packet ...
  p.flush(ts, socket);

  auto hdrsize = jb::itch5::mold_udp_protocol::header_size;
  // ... that should immediately flush the first two messages ...
  BOOST_REQUIRE_EQUAL(socket.packets.size(), 1);
  BOOST_CHECK_EQUAL(
      hdrsize + 100 + 2, socket.packets.at(0).size());

  // ... this flush() request should result in no more packets ...
  p.flush(ts, socket);
  BOOST_REQUIRE_EQUAL(socket.packets.size(), 1);

  // ... while a heartbeat() request should result in an additional
  // packet ...
  p.heartbeat(socket);
  BOOST_REQUIRE_EQUAL(socket.packets.size(), 2);
  BOOST_CHECK_EQUAL(hdrsize, socket.packets.at(1).size());
}

/**
 * @test Increase code coverage in jb::itch5::testing::create_message()
 */
BOOST_AUTO_TEST_CASE(itch5_testing_create_message_errors) {
  jb::itch5::timestamp ts{std::chrono::microseconds(1000)};

  // message too small ...
  BOOST_CHECK_THROW(
      jb::itch5::testing::create_message('A', ts, 2), std::exception);

  // ... message too big ...
  BOOST_CHECK_THROW(
      jb::itch5::testing::create_message('A', ts, 100000), std::exception);

  // ... message type out of range ...
  BOOST_CHECK_THROW(
      jb::itch5::testing::create_message(-1, ts, 100), std::exception);
  BOOST_CHECK_THROW(
      jb::itch5::testing::create_message(256, ts, 100), std::exception);
}
