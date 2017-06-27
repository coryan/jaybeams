#include "jb/etcd/grpc_errors.hpp"
#include <etcd/etcdserver/etcdserverpb/rpc.pb.h>

#include <boost/test/unit_test.hpp>
#include <sstream>

/**
 * @test Verify that print_to_stream works as expected.
 */
BOOST_AUTO_TEST_CASE(print_to_stream_basic) {
  etcdserverpb::LeaseKeepAliveRequest req;
  req.set_id(42);

  std::string expected = R"""(ID: 42
)""";
  std::ostringstream os;
  os << jb::etcd::print_to_stream(req);
  auto actual = os.str();
  BOOST_CHECK_EQUAL(actual, expected);
}
