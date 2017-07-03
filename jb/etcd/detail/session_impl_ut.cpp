#include "jb/etcd/detail/session_impl.hpp"

#include <jb/etcd/detail/mocked_grpc_interceptor.hpp>
#include <boost/test/unit_test.hpp>

/// Define helper types and functions used in these tests
namespace {
using completion_queue_type =
    jb::etcd::completion_queue<jb::etcd::detail::mocked_grpc_interceptor>;
using session_type = jb::etcd::detail::session_impl<completion_queue_type>;

/// Common initialization for all tests
void prepare_mocks_common(completion_queue_type& queue);
} // anonymous namespace

/**
 * @test Verify that jb::etcd::detail::session_impl works in the
 * simple case.
 */
BOOST_AUTO_TEST_CASE(session_basic) {
  using namespace std::chrono_literals;
  using namespace jb::etcd::detail;

  completion_queue_type queue;
  prepare_mocks_common(queue);
  using namespace ::testing;
  // ... we expect a call to request a lease ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(Truly([](auto op) {
    return op->name == "session/preamble/lease_grant";
  }))).WillOnce(Invoke([](auto bop) {
    using op_type = jb::etcd::detail::async_op<
        etcdserverpb::LeaseGrantRequest, etcdserverpb::LeaseGrantResponse>;
    auto* op = dynamic_cast<op_type*>(bop.get());
    BOOST_REQUIRE(op != nullptr);
    // ... verify the request is what we expect ...
    BOOST_REQUIRE_EQUAL(op->request.ttl(), 5);
    BOOST_REQUIRE_EQUAL(op->request.id(), 0);

    // ... in this test we just assume everything worked, so
    // provide a response ...
    op->response.set_error("");
    op->response.set_id(1000);
    op->response.set_ttl(42);
    // ... and to not forget the callback ...
    bop->callback(*bop, true);
  }));

  // ... and a call to setup a timer ...
  std::shared_ptr<deadline_timer> pending_timer;
  EXPECT_CALL(
      *queue.interceptor().shared_mock, make_deadline_timer(Truly([](auto op) {
        return op->name == "session/set_timer/ttl_refresh";
      })))
      .WillOnce(Invoke([r = std::ref(pending_timer)](auto bop) {
        using op_type = jb::etcd::detail::deadline_timer;
        auto* op = dynamic_cast<op_type*>(bop.get());
        BOOST_REQUIRE(op != nullptr);
        // ... the standard trick to downcast shared_ptr<> ...
        r.get() = std::shared_ptr<deadline_timer>(bop, op);
        // ... delay sending the callback until we can test it:
        //  DO NOT CALL YET bop->callback(*bop, true);
      }));

  auto session = std::make_unique<session_type>(
      queue, std::unique_ptr<etcdserverpb::Lease::Stub>(), 5000ms);
  BOOST_CHECK_EQUAL(session->lease_id(), 1000);
  BOOST_CHECK_EQUAL(session->actual_TTL().count(), 42000);
  BOOST_CHECK_NO_THROW(session.reset(nullptr));
}

/**
 * @test Verify that jb::etcd::detail::session_impl works when the
 * lease fails to be acquired.
 */
BOOST_AUTO_TEST_CASE(session_lease_error) {
  using namespace std::chrono_literals;
  using namespace jb::etcd::detail;

  completion_queue_type queue;
  prepare_mocks_common(queue);
  using namespace ::testing;
  // ... we expect a call to request a lease ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(Truly([](auto op) {
    return op->name == "session/preamble/lease_grant";
  }))).WillOnce(Invoke([](auto bop) {
    using op_type = jb::etcd::detail::async_op<
        etcdserverpb::LeaseGrantRequest, etcdserverpb::LeaseGrantResponse>;
    auto* op = dynamic_cast<op_type*>(bop.get());
    BOOST_REQUIRE(op != nullptr);
    // ... verify the request is what we expect ...
    BOOST_REQUIRE_EQUAL(op->request.ttl(), 5);
    BOOST_REQUIRE_EQUAL(op->request.id(), 0);

    // ... in this test we just assume everything worked, so
    // provide a response ...
    op->response.set_error("something broke");
    // ... and to not forget the callback ...
    bop->callback(*bop, true);
  }));

  // ... there should be no calls to setup the timer.
  std::shared_ptr<deadline_timer> pending_timer;
  EXPECT_CALL(
      *queue.interceptor().shared_mock, make_deadline_timer(Truly([](auto op) {
        return op->name == "session/set_timer/ttl_refresh";
      })))
    .Times(0);

  std::unique_ptr<session_type> session;

  BOOST_CHECK_THROW(
      session = std::make_unique<session_type>(
          queue, std::unique_ptr<etcdserverpb::Lease::Stub>(), 5000ms),
      std::exception);
}

/**
 * @test Verify that jb::etcd::detail::session_impl works when the
 * lease fails to be acquired.
 */
BOOST_AUTO_TEST_CASE(session_lease_unusual_exception) {
  using namespace std::chrono_literals;
  using namespace jb::etcd::detail;

  completion_queue_type queue;
  prepare_mocks_common(queue);
  using namespace ::testing;
  // ... we expect a call to request a lease ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(Truly([](auto op) {
    return op->name == "session/preamble/lease_grant";
  }))).WillOnce(Invoke([](auto bop) {
    throw std::string("raising std::string is just bad manners");
  }));

  // ... there should be no calls to setup the timer.
  std::shared_ptr<deadline_timer> pending_timer;
  EXPECT_CALL(
      *queue.interceptor().shared_mock, make_deadline_timer(Truly([](auto op) {
        return op->name == "session/set_timer/ttl_refresh";
      })))
    .Times(0);

  std::unique_ptr<session_type> session;

  BOOST_CHECK_THROW(
      session = std::make_unique<session_type>(
          queue, std::unique_ptr<etcdserverpb::Lease::Stub>(), 5000ms),
      std::string);
}

namespace {
void prepare_mocks_common(completion_queue_type& queue) {
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
  EXPECT_CALL(*queue.interceptor().shared_mock, make_deadline_timer(_))
      .WillRepeatedly(Invoke([](auto op) { op->callback(*op, true); }));
}
} // anonymous namespace