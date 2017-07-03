#include <jb/itch5/udp_receiver_config.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::itch5::udp_receiver_config works as expected.
 */
BOOST_AUTO_TEST_CASE(itch5_udp_receiver_config_basic) {
  BOOST_CHECK_NO_THROW(jb::itch5::udp_receiver_config());
  BOOST_CHECK_NO_THROW(
      jb::itch5::udp_receiver_config().address("127.0.0.1").validate());
  BOOST_CHECK_NO_THROW(
      jb::itch5::udp_receiver_config().address("::1").validate());

  BOOST_CHECK_NO_THROW(jb::itch5::udp_receiver_config()
                           .address("239.128.1.1")
                           .local_address("127.0.0.1")
                           .validate());
  BOOST_CHECK_NO_THROW(jb::itch5::udp_receiver_config()
                           .address("ff05::")
                           .local_address("::1")
                           .validate());

  BOOST_CHECK_THROW(
      jb::itch5::udp_receiver_config()
          .address("127.0.0.1")
          .local_address("127.0.0.2")
          .validate(),
      jb::usage);
  BOOST_CHECK_THROW(
      jb::itch5::udp_receiver_config()
          .address("::1")
          .local_address("127.0.0.1")
          .validate(),
      jb::usage);
}
