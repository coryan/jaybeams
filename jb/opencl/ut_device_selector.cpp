#include <jb/opencl/device_selector.hpp>
#include <jb/opencl/config.hpp>

#include <boost/compute/system.hpp>
#include <boost/test/unit_test.hpp>

/**
 * @test Verify that we can select devices by name.
 */
BOOST_AUTO_TEST_CASE(opencl_device_selector_by_name) {
  auto devices = boost::compute::system::devices();
  for (auto const& d : devices) {
    BOOST_MESSAGE("searching for " << d.name());
    boost::compute::device actual = jb::opencl::device_selector(
        jb::opencl::config().device_name(d.name()));
      
    BOOST_CHECK_EQUAL(d.name(), actual.name());
    BOOST_CHECK_EQUAL(d.id(), actual.id());
  }
}

/**
 * @test Verify that the default selection works as expected.
 */
BOOST_AUTO_TEST_CASE(opencl_device_selector_default) {
  auto expected = boost::compute::system::default_device();

  auto actual = jb::opencl::device_selector(jb::opencl::config());
  BOOST_MESSAGE("Default selector picked " << actual.name());

  BOOST_CHECK_EQUAL(expected.name(), actual.name());
  BOOST_CHECK_EQUAL(expected.id(), actual.id());
}

/**
 * @test Verify that the default selection works as expected.
 */
BOOST_AUTO_TEST_CASE(opencl_device_selector_bestcpu) {
  auto actual = jb::opencl::device_selector(
      jb::opencl::config().device_name("BESTCPU"));
  BOOST_MESSAGE("Default selector picked " << actual.name());

  for (auto const& d : boost::compute::system::devices()) {
    if (d.type() == boost::compute::device::cpu) {
      BOOST_MESSAGE("checking compute unit count for " << d.name());
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
  } catch(std::exception const& ex) {
    BOOST_MESSAGE("No available GPU, abort test");
    return;
  }
  BOOST_MESSAGE("Default selector picked " << actual.name());

  for (auto const& d : boost::compute::system::devices()) {
    if (d.type() == boost::compute::device::gpu) {
      BOOST_MESSAGE("checking compute unit count for " << d.name());
      BOOST_CHECK_GE(actual.compute_units(), d.compute_units());
    }
  }
}

/**
 * @test Verify that the device selector without config works as expected.
 */
BOOST_AUTO_TEST_CASE(opencl_device_selector_no_config) {
  auto expected = jb::opencl::device_selector(
      jb::opencl::config());
  auto actual = jb::opencl::device_selector();

  BOOST_CHECK_EQUAL(expected.id(), actual.id());
  BOOST_CHECK_EQUAL(expected.name(), actual.name());
}
