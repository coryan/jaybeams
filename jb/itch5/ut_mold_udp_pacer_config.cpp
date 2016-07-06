#include <jb/itch5/mold_udp_pacer_config.hpp>

#include <chrono>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::itch5::mold_udp_pacer_config validation works
 * as expected (also increase code coverage!).
 */
BOOST_AUTO_TEST_CASE(itch5_mold_udp_pacer_config_validate) {
  using config = jb::itch5::mold_udp_pacer_config;

  config default_validates;
  BOOST_CHECK_NO_THROW(default_validates.validate());

  config mtu_too_small = config().maximum_transmission_unit(8);
  BOOST_CHECK_THROW(mtu_too_small.validate(), jb::usage);

  config mtu_too_big = config().maximum_transmission_unit(100000);
  BOOST_CHECK_THROW(mtu_too_big.validate(), jb::usage);

  config delay_too_small = config().maximum_delay_microseconds(0);
  BOOST_CHECK_THROW(delay_too_small.validate(), jb::usage);

  using namespace std::chrono;
  config delay_too_big = config().maximum_delay_microseconds(
      duration_cast<microseconds>(minutes(5)).count());
  BOOST_CHECK_THROW(delay_too_big.validate(), jb::usage);
}
