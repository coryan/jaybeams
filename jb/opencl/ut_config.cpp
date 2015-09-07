#include <jb/opencl/config.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that the cl_config works as expected.
 */
BOOST_AUTO_TEST_CASE(opencl_config_basic) {
  jb::opencl::config config;

  BOOST_CHECK_EQUAL(config.device_name(), "");

  config
      .device_name("foo")
      .device_name("bar")
      ;

  BOOST_CHECK_EQUAL(config.device_name(), "bar");
}

BOOST_AUTO_TEST_CASE(cl_config_cmdline) {
  char argv0[] = "a/b/c";
  char argv1[] = "--device-name=Tahiti";
  char *argv[] = {argv0, argv1};
  int argc = sizeof(argv) / sizeof(argv[0]);

  jb::opencl::config config;
  config.process_cmdline(argc, argv);

  BOOST_CHECK_EQUAL(config.device_name(), "Tahiti");
}
