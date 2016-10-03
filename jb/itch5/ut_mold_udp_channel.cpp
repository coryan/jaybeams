#include <jb/itch5/mold_udp_channel.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::itch5::mold_udp_channel can be compiled.
 */
BOOST_AUTO_TEST_CASE(itch5_mold_udp_channel_basic) {
  auto handler = [](
      std::chrono::steady_clock::time_point, std::uint64_t, std::size_t,
      char const*, std::size_t) {
  };

  boost::asio::io_service io;
  jb::itch5::mold_udp_channel channel(handler, io, "", 50000, "::1");
}
