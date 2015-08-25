#include <jb/filetype.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify jb::is_gz works as expected.
 */
BOOST_AUTO_TEST_CASE(is_gz_basic) {

  BOOST_CHECK_EQUAL(false, jb::is_gz(""));
  BOOST_CHECK_EQUAL(false, jb::is_gz("foo.Z"));
  BOOST_CHECK_EQUAL(false, jb::is_gz("foo.gz.Z"));
  BOOST_CHECK_EQUAL(false, jb::is_gz("bar.gz/foo"));
  BOOST_CHECK_EQUAL(false, jb::is_gz("bar/foo"));
  BOOST_CHECK_EQUAL(false, jb::is_gz(".gz"));

  BOOST_CHECK_EQUAL(true, jb::is_gz("foo.gz"));
  BOOST_CHECK_EQUAL(true, jb::is_gz("bar/foo.gz"));
  BOOST_CHECK_EQUAL(true, jb::is_gz("bar/baz/foo.gz"));
}
