#include "jb/etcd/leader_election_runner.hpp"

#include <jb/etcd/detail/mocked_grpc_interceptor.hpp>
#include <jb/etcd/grpc_errors.hpp>
#include <jb/etcd/prefix_end.hpp>
#include <jb/testing/future_status.hpp>

#include <boost/test/unit_test.hpp>

#include <google/protobuf/text_format.h>
#include <sstream>
#include <stdexcept>

namespace jb {
namespace etcd {




} // namespace etcd
} // namespace jb

/**
 * @test Verify that jb::etcd::leader_election_runner works in the
 * simple case.
 */
BOOST_AUTO_TEST_CASE(leader_election_runner_basic) {
  using namespace std::chrono_literals;

  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;
  using completion_queue_type =
      completion_queue<detail::mocked_grpc_interceptor>;

  completion_queue_type queue;

  // The sequences of queries for a leader election participant is
  // rather long, but it goes like this ...

  using namespace ::testing;

  // ... on most calls we just invoke the application's callback
  // immediately ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(_))
      .WillRepeatedly(Invoke([](auto op) { op->callback(*op, true); }));
  EXPECT_CALL(*queue.interceptor().shared_mock, async_read(_))
      .WillRepeatedly(Invoke([](auto op) { op->callback(*op, true); }));
  EXPECT_CALL(*queue.interceptor().shared_mock, async_write(_))
      .WillRepeatedly(Invoke([](auto op) { op->callback(*op, true); }));
  EXPECT_CALL(*queue.interceptor().shared_mock, async_create_rdwr_stream(_))
      .WillRepeatedly(Invoke([](auto op) { op->callback(*op, true); }));
  EXPECT_CALL(*queue.interceptor().shared_mock, async_writes_done(_))
      .WillRepeatedly(Invoke([](auto op) { op->callback(*op, true); }));
  EXPECT_CALL(*queue.interceptor().shared_mock, async_finish(_))
      .WillRepeatedly(Invoke([](auto op) { op->callback(*op, true); }));

  // ... the class will try to create a node for the participant using
  // async_rpc, provide a good response for it ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(Truly([](auto op) {
    return op->name == "leader_election/commit/create_node";
  }))).WillOnce(Invoke([](auto bop) {
    using op_type =
        detail::async_op<etcdserverpb::TxnRequest, etcdserverpb::TxnResponse>;
    auto* op = dynamic_cast<op_type*>(bop.get());
    BOOST_REQUIRE(op != nullptr);
    // ... verify the request is what we expect ...
    BOOST_REQUIRE_EQUAL(op->request.compare().size(), 1UL);
    auto const& cmp = op->request.compare()[0];
    BOOST_CHECK_EQUAL(cmp.key(), "test-election/123456");
    BOOST_REQUIRE_EQUAL(op->request.success().size(), 1UL);
    auto const& success = op->request.success()[0];
    BOOST_CHECK_EQUAL(success.request_put().key(), "test-election/123456");
    BOOST_CHECK_EQUAL(success.request_put().value(), "mocked-runner-a");
    BOOST_CHECK_EQUAL(success.request_put().lease(), 0x123456);

    // ... in this test we just assume everything worked, so
    // provide a response ...
    op->response.set_succeeded(true);
    op->response.mutable_header()->set_revision(2345678UL);
    // ... and to not forget the callback ...
    bop->callback(*bop, true);
  }));

  // ... shortly after creating a node, the class will request the
  // range of other nodes with the same prefix ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(Truly([](auto op) {
    return op->name == "leader_election_participant/campaign/range";
  }))).WillOnce(Invoke([](auto bop) {
    using op_type = detail::async_op<
        etcdserverpb::RangeRequest, etcdserverpb::RangeResponse>;
    auto* op = dynamic_cast<op_type*>(bop.get());
    BOOST_REQUIRE(op != nullptr);
    // ... verify the request is what we expect ...
    BOOST_REQUIRE_EQUAL(op->request.key(), "test-election/");
    BOOST_REQUIRE_EQUAL(op->request.range_end(), "test-election0");
    // ... the magic number is provided by the previous mocked call ...
    BOOST_REQUIRE_EQUAL(op->request.max_create_revision(), 2345677UL);
    BOOST_REQUIRE_EQUAL(
        op->request.sort_order(), etcdserverpb::RangeRequest::DESCEND);
    BOOST_REQUIRE_EQUAL(
        op->request.sort_target(), etcdserverpb::RangeRequest::CREATE);
    BOOST_REQUIRE_EQUAL(op->request.limit(), 1);

    // ... in this test we just assume everything is simple, just
    // provide an empty response ...

    // ... and to not forget the callback ...
    bop->callback(*bop, true);
  }));

  using runner_type = leader_election_runner<completion_queue_type>;

  bool elected = false;
  runner_type runner(
      queue, 0x123456UL, std::unique_ptr<etcdserverpb::KV::Stub>(),
      std::unique_ptr<etcdserverpb::Watch::Stub>(),
      std::string("test-election"), std::string("mocked-runner-a"),
      [&elected](std::future<bool>& src) { elected = src.get(); });
  BOOST_CHECK_EQUAL(elected, true);
}
