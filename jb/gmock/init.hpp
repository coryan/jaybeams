/**
 * @file
 *
 * Initialize GMock to work with Boost.Test
 */
#ifndef jb_gmock_init_hpp
#define jb_gmock_init_hpp

#include <gmock/gmock.h>
#include <boost/test/unit_test.hpp>

namespace jb {
namespace gmock {

/// A Boost.Test global fixture to initialize GMock.
struct initialize_from_boost_test {
    initialize_from_boost_test() {
      ::testing::GTEST_FLAG(throw_on_failure) = true;
      ::testing::InitGoogleMock(
          &boost::unit_test::framework::master_test_suite().argc,
          boost::unit_test::framework::master_test_suite().argv);
    }
    ~initialize_from_boost_test() { }
};
BOOST_GLOBAL_FIXTURE(initialize_from_boost_test);

} // namespace gmock
} // namespace jb

#endif // jb_gmock_init_hpp
