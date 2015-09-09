#include <jb/opencl/device_selector.hpp>
#include <jb/opencl/config.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that we can select devices by name.
 */
BOOST_AUTO_TEST_CASE(opencl_device_selector_by_name) {

  {
    cl::Context context(CL_DEVICE_TYPE_CPU, 0, nullptr, nullptr);

    auto devices = context.getInfo<CL_CONTEXT_DEVICES>();
    for (auto const& d : devices) {
      std::string expected = d.getInfo<CL_DEVICE_NAME>();
      BOOST_MESSAGE("searching for " << expected);
      cl::Device dev = jb::opencl::device_selector(
          jb::opencl::config().device_name(expected));
      
      BOOST_CHECK_EQUAL(dev.getInfo<CL_DEVICE_NAME>(), expected);
    }
  }

  std::vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);
  for (auto const& p : platforms) {
    std::vector<cl::Device> devices;
    p.getDevices(CL_DEVICE_TYPE_ALL, &devices);

    for (auto const& d : devices) {
      std::string expected = d.getInfo<CL_DEVICE_NAME>();
      BOOST_MESSAGE("searching for " << expected);
      cl::Device dev = jb::opencl::device_selector(
          jb::opencl::config().device_name(expected));
      
      BOOST_CHECK_EQUAL(dev.getInfo<CL_DEVICE_NAME>(), expected);
    }
  }
}

/**
 * @test Verify that the default selection works as expected.
 */
BOOST_AUTO_TEST_CASE(opencl_device_selector_default) {

  cl::Device dev = jb::opencl::device_selector(jb::opencl::config());
  BOOST_MESSAGE("Default selector picked " << dev.getInfo<CL_DEVICE_NAME>());

  BOOST_CHECK_NO_THROW(dev.getInfo<CL_DEVICE_NAME>());

  bool has_gpu = false;

  std::vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);
  for (auto const& p : platforms) {
    std::vector<cl::Device> devices;
    p.getDevices(CL_DEVICE_TYPE_ALL, &devices);

    for (auto const& d : devices) {
      if (d.getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_GPU) {
        has_gpu = true;
        BOOST_CHECK_EQUAL(dev.getInfo<CL_DEVICE_TYPE>(), CL_DEVICE_TYPE_GPU);
        BOOST_CHECK_GE(
            dev.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>(),
            d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>());
      }
    }
  }

  if (has_gpu) {
    return;
  }

  BOOST_CHECK_EQUAL(dev.getInfo<CL_DEVICE_TYPE>(), CL_DEVICE_TYPE_CPU);
  for (auto const& p : platforms) {
    std::vector<cl::Device> devices;
    p.getDevices(CL_DEVICE_TYPE_ALL, &devices);

    for (auto const& d : devices) {
      if (d.getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU) {
        BOOST_CHECK_GE(
            dev.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>(),
            d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>());
      }
    }
  }
}

/**
 * @test Verify that the default selection works as expected.
 */
BOOST_AUTO_TEST_CASE(opencl_device_selector_bestcpu) {
  cl::Device dev = jb::opencl::device_selector(
      jb::opencl::config().device_name("BESTCPU"));
  BOOST_MESSAGE("Default selector picked " << dev.getInfo<CL_DEVICE_NAME>());

  BOOST_CHECK_NO_THROW(dev.getInfo<CL_DEVICE_NAME>());

  std::vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);
  for (auto const& p : platforms) {
    std::vector<cl::Device> devices;
    p.getDevices(CL_DEVICE_TYPE_CPU, &devices);

    for (auto const& d : devices) {
      BOOST_CHECK_EQUAL(dev.getInfo<CL_DEVICE_TYPE>(), CL_DEVICE_TYPE_CPU);
      BOOST_CHECK_GE(
          dev.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>(),
          d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>());
    }
  }
}

/**
 * @test Verify that the default selection works as expected.
 */
BOOST_AUTO_TEST_CASE(opencl_device_selector_bestgpu) {
  cl::Device dev = jb::opencl::device_selector(
      jb::opencl::config().device_name("BESTGPU"));
  BOOST_MESSAGE("Default selector picked " << dev.getInfo<CL_DEVICE_NAME>());

  BOOST_CHECK_NO_THROW(dev.getInfo<CL_DEVICE_NAME>());

  std::vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);
  for (auto const& p : platforms) {
    std::vector<cl::Device> devices;
    p.getDevices(CL_DEVICE_TYPE_GPU, &devices);

    for (auto const& d : devices) {
      BOOST_CHECK_EQUAL(dev.getInfo<CL_DEVICE_TYPE>(), CL_DEVICE_TYPE_GPU);
      BOOST_CHECK_GE(
          dev.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>(),
          d.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>());
    }
  }
}

/**
 * @test Verify that the device selector without config works as expected.
 */
BOOST_AUTO_TEST_CASE(opencl_device_selector_no_config) {
  cl::Device expected = jb::opencl::device_selector(
      jb::opencl::config());
  cl::Device got = jb::opencl::device_selector();

  BOOST_CHECK_EQUAL(
      got.getInfo<CL_DEVICE_NAME>(), expected.getInfo<CL_DEVICE_NAME>());
}
