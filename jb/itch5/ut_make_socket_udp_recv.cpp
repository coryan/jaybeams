#include <jb/itch5/make_socket_udp_recv.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::itch5::make_socket_udp_recv compiles.
 */
BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_recv_compile) {
  boost::asio::io_service io;
  auto socket = jb::itch5::make_socket_udp_recv(io, "0.0.0.0", 50000, "");
  BOOST_CHECK(socket.is_open());
}

/**
 * Types used in testing of jb::itch5::make_socket_udp_recv
 */
namespace {
} // anonymous namespace

BOOST_AUTO_TEST_CASE(itch5_make_socket_udp_recv_basic) {
  boost::asio::io_service io;
}
