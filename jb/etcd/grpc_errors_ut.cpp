#include "jb/etcd/grpc_errors.hpp"
#include <etcd/etcdserver/etcdserverpb/rpc.pb.h>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that check_rpc_status works as expected.
 */
BOOST_AUTO_TEST_CASE(check_grpc_status_ok) {
  grpc::Status status = grpc::Status::OK;

  BOOST_CHECK_NO_THROW(jb::etcd::check_grpc_status(status, "test"));

  etcdserverpb::LeaseKeepAliveRequest req;
  BOOST_CHECK_NO_THROW(jb::etcd::check_grpc_status(
      status, "test", " in iteration=", 42,
      ", request=", jb::etcd::print_to_stream(req)));
}

/**
 * @test Verify that check_rpc_status throws what is expected.
 */
BOOST_AUTO_TEST_CASE(check_grpc_status_error_annotations) {
  try {
    grpc::Status status(grpc::UNKNOWN, "bad thing");
    etcdserverpb::LeaseKeepAliveRequest req;
    req.set_id(42);
    jb::etcd::check_grpc_status(
        status, "test", " request=", jb::etcd::print_to_stream(req));
  } catch (std::runtime_error const& ex) {
    std::string const expected =
        R"""(test grpc error: bad thing [2] request=ID: 42
)""";
    BOOST_CHECK_EQUAL(ex.what(), expected);
  }
}

/**
 * @test Verify that check_rpc_status throws what is expected.
 */
BOOST_AUTO_TEST_CASE(check_grpc_status_error_bare) {
  try {
    grpc::Status status(grpc::UNKNOWN, "bad thing");
    etcdserverpb::LeaseKeepAliveRequest req;
    req.set_id(42);
    jb::etcd::check_grpc_status(status, "test");
  } catch (std::runtime_error const& ex) {
    std::string const expected =
        R"""(test grpc error: bad thing [2])""";
    BOOST_CHECK_EQUAL(ex.what(), expected);
  }
}

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
