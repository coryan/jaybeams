#include <jb/opencl/config.hpp>
#include <jb/opencl/device_selector.hpp>

#include <boost/compute/system.hpp>
#include <boost/test/unit_test.hpp>

/**
 * @test Verify that we can select devices by name.
 */
BOOST_AUTO_TEST_CASE(opencl_device_selector_by_name) {
  auto devices = boost::compute::system::devices();
  for (auto const& d : devices) {
    BOOST_TEST_MESSAGE("searching for " << d.name());
    boost::compute::device actual =
        jb::opencl::device_selector(jb::opencl::config().device_name(d.name()));

    BOOST_CHECK_EQUAL(d.name(), actual.name());
    BOOST_CHECK_EQUAL(d.id(), actual.id());
  }
}

/**
 * @test Verify that the selection with an empty name works as expected.
 */
BOOST_AUTO_TEST_CASE(opencl_device_selector_empty) {
  auto actual = jb::opencl::device_selector(jb::opencl::config());
  BOOST_TEST_MESSAGE("Default selector picked " << actual.name());

  for (auto const& d : boost::compute::system::devices()) {
    BOOST_TEST_MESSAGE("checking compute unit count for " << d.name());
    BOOST_CHECK_GE(actual.compute_units(), d.compute_units());
  }
}

/**
 * @test Verify that the default selection works as expected.
 */
BOOST_AUTO_TEST_CASE(opencl_device_selector_bestcpu) {
  boost::compute::device actual;
  try {
    actual = jb::opencl::device_selector(
        jb::opencl::config().device_name("BESTCPU"));
  } catch (std::exception const& ex) {
    // On some platforms (pocl) there are no CPU devices, weird.
    BOOST_TEST_MESSAGE("No available CPU, abort test");
    return;
  }
  BOOST_TEST_MESSAGE("Default selector picked " << actual.name());

  for (auto const& d : boost::compute::system::devices()) {
    if (d.type() & boost::compute::device::cpu) {
      BOOST_TEST_MESSAGE("checking compute unit count for " << d.name());
      BOOST_CHECK_GE(actual.compute_units(), d.compute_units());
    }
  }
}

/**
 * @test Verify that the default selection works as expected.
 */
BOOST_AUTO_TEST_CASE(opencl_device_selector_bestgpu) {
  boost::compute::device actual;
  try {
    actual = jb::opencl::device_selector(
        jb::opencl::config().device_name("BESTGPU"));
  } catch (std::exception const& ex) {
    // On machines without a GPU we do not want to fail the test, the
    // expected result is that no device is found ...
    BOOST_TEST_MESSAGE("No available GPU, abort test");
    return;
  }
  BOOST_TEST_MESSAGE("Default selector picked " << actual.name());

  for (auto const& d : boost::compute::system::devices()) {
    if (d.type() & boost::compute::device::gpu) {
      BOOST_TEST_MESSAGE("checking compute unit count for " << d.name());
      BOOST_CHECK_GE(actual.compute_units(), d.compute_units());
    }
  }
}

/**
 * @test Verify that the device selector without config works as expected.
 */
BOOST_AUTO_TEST_CASE(opencl_device_selector_no_config) {
  auto actual = jb::opencl::device_selector();
  auto expected = jb::opencl::device_selector(jb::opencl::config());

  BOOST_CHECK_EQUAL(expected.id(), actual.id());
  BOOST_CHECK_EQUAL(expected.name(), actual.name());
  BOOST_TEST_MESSAGE(
      "Default device name=" << actual.name() << ", id=" << actual.id()
                             << ", type=" << actual.type());
}

/**
 * @test Verify that the device selector for "SYSTEM:DEFAULT" works as expected.
 */
BOOST_AUTO_TEST_CASE(opencl_device_selector_system_default) {
  auto actual = jb::opencl::device_selector(
      jb::opencl::config().device_name("SYSTEM:DEFAULT"));
  auto expected = boost::compute::system::default_device();

  BOOST_CHECK_EQUAL(expected.id(), actual.id());
  BOOST_CHECK_EQUAL(expected.name(), actual.name());
}

/**
 * @test Verify that detail::best_device() helper fails as expected.
 */
BOOST_AUTO_TEST_CASE(opencl_device_selector_filter_failure) {
  using boost::compute::device;
  BOOST_CHECK_THROW(
      jb::opencl::detail::best_device(
          [](device const&) { return false; }, "FAIL"),
      std::runtime_error);
  BOOST_CHECK_NO_THROW(
      jb::opencl::detail::best_device(
          [](device const&) { return true; }, "ANY"));
}
