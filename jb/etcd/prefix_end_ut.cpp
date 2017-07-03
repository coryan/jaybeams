#include "jb/etcd/prefix_end.hpp"

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::etcd::prefix_end works as expected.
 */
BOOST_AUTO_TEST_CASE(leader_election_runner_basic) {
  BOOST_CHECK_EQUAL(u'/' + 1, u'0');
  BOOST_CHECK_EQUAL(jb::etcd::prefix_end("foo/"), std::string("foo0"));
  std::string actual = jb::etcd::prefix_end(u8"\xFF\xFF");
  char buf0[] = u8"\x00\x00\x01";
  std::string expected(buf0, buf0 + sizeof(buf0) - 1);
  BOOST_CHECK_EQUAL_COLLECTIONS(
      actual.begin(), actual.end(), expected.begin(), expected.end());

  actual = jb::etcd::prefix_end(u8"ABC\xFF");
  char buf1[] = u8"ABD\x00";
  expected.assign(buf1, buf1 + sizeof(buf1) - 1);
  BOOST_CHECK_EQUAL_COLLECTIONS(
      actual.begin(), actual.end(), expected.begin(), expected.end());
}
