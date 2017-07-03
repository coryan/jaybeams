#include "jb/etcd/leader_election_runner.hpp"

#include <jb/etcd/detail/mocked_grpc_interceptor.hpp>
#include <jb/etcd/grpc_errors.hpp>
#include <jb/etcd/prefix_end.hpp>
#include <jb/testing/future_status.hpp>

#include <boost/test/unit_test.hpp>

#include <google/protobuf/text_format.h>
#include <sstream>
#include <stdexcept>

/// Define helper types and functions used in these tests
namespace {
using completion_queue_type =
    jb::etcd::completion_queue<jb::etcd::detail::mocked_grpc_interceptor>;
using runner_type = jb::etcd::leader_election_runner<completion_queue_type>;

/// Common initialization for all tests
void prepare_mocks_common(completion_queue_type& queue);

/// Run a basic test where the runner starts in the elected state.
void prepare_mocks_for_initially_elected(completion_queue_type& queue);

/// Run a basic test where the runner does not start in the elected state.
void prepare_mocks_for_not_initially_elected(completion_queue_type& queue);

/// Setup the mocks assuming the steps in the create_node phase are setup
void prepare_mocks_for_initially_elected_post_node(
    completion_queue_type& queue, int revision);
}

/**
 * @test Verify that jb::etcd::leader_election_runner works in the
 * simple case.
 */
BOOST_AUTO_TEST_CASE(leader_election_runner_basic) {
  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;

  completion_queue_type queue;

  prepare_mocks_for_initially_elected(queue);

  bool elected = false;
  auto runner = std::make_unique<runner_type>(
      queue, 0x123456UL, std::unique_ptr<etcdserverpb::KV::Stub>(),
      std::unique_ptr<etcdserverpb::Watch::Stub>(),
      std::string("test-election"), std::string("mocked-runner-a"),
      [&elected](std::future<bool>& src) { elected = src.get(); });
  BOOST_CHECK_EQUAL(elected, true);

  BOOST_CHECK_NO_THROW(runner.reset(nullptr));
}

/**
 * @test Verify that jb::etcd::leader_election_runner can publish new
 * values after it is elected.
 */
BOOST_AUTO_TEST_CASE(leader_election_runner_proclaim) {
  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;

  completion_queue_type queue;

  prepare_mocks_for_initially_elected(queue);

  bool elected = false;
  auto runner = std::make_unique<runner_type>(
      queue, 0x123456UL, std::unique_ptr<etcdserverpb::KV::Stub>(),
      std::unique_ptr<etcdserverpb::Watch::Stub>(),
      std::string("test-election"), std::string("mocked-runner-a"),
      [&elected](std::future<bool>& src) { elected = src.get(); });
  BOOST_CHECK_EQUAL(elected, true);

  // ... when we call proclaim() that translates into a RPC, prepare
  // the system to handle it ...
  using namespace ::testing;
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(Truly([](auto op) {
    return op->name == "leader_election/publish_value";
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
    BOOST_CHECK_EQUAL(
        success.request_put().value(), "mocked-runner-a has moved");
    BOOST_CHECK_EQUAL(success.request_put().lease(), 0x123456);

    // ... first simulate a successful call ...
    op->response.set_succeeded(true);
    op->response.mutable_header()->set_revision(2345679UL);
    // ... and to not forget the callback ...
    bop->callback(*bop, true);
  }));

  // ... run the operation ...
  BOOST_CHECK_NO_THROW(runner->proclaim("mocked-runner-a has moved"));
  BOOST_CHECK_EQUAL(runner->value(), "mocked-runner-a has moved");

  // ... prepare a second operation, but this time make it fail ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(Truly([](auto op) {
    return op->name == "leader_election/publish_value";
  }))).WillOnce(Invoke([](auto bop) {
    using op_type =
        detail::async_op<etcdserverpb::TxnRequest, etcdserverpb::TxnResponse>;
    auto* op = dynamic_cast<op_type*>(bop.get());
    BOOST_REQUIRE(op != nullptr);
    op->response.set_succeeded(false);
    bop->callback(*bop, true);
  }));

  // ... and then ...
  BOOST_CHECK_THROW(
      runner->proclaim("mocked-runner-a moved again"), std::exception);
  BOOST_CHECK_EQUAL(runner->value(), "mocked-runner-a has moved");

  // ... also if the operation gets canceled ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(Truly([](auto op) {
    return op->name == "leader_election/publish_value";
  }))).WillOnce(Invoke([](auto bop) { bop->callback(*bop, false); }));

  // ... and then ...
  BOOST_CHECK_THROW(
      runner->proclaim("mocked-runner-a wants to move!"), std::exception);
  BOOST_CHECK_EQUAL(runner->value(), "mocked-runner-a has moved");

  BOOST_CHECK_NO_THROW(runner.reset(nullptr));
}

/**
 * @test Verify that jb::etcd::leader_election_runner can resign after
 * elected.
 */
BOOST_AUTO_TEST_CASE(leader_election_runner_resign) {
  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;

  completion_queue_type queue;

  prepare_mocks_for_initially_elected(queue);

  bool elected = false;
  auto runner = std::make_unique<runner_type>(
      queue, 0x123456UL, std::unique_ptr<etcdserverpb::KV::Stub>(),
      std::unique_ptr<etcdserverpb::Watch::Stub>(),
      std::string("test-election"), std::string("mocked-runner-a"),
      [&elected](std::future<bool>& src) { elected = src.get(); });
  BOOST_CHECK_EQUAL(elected, true);

  // ... when we call resign() we cancel all watchers, but there are
  // none in this case ...
  using namespace ::testing;
  EXPECT_CALL(*queue.interceptor().shared_mock, async_write(Truly([](auto op) {
    return op->name == "leader_election/publish_value";
  }))).Times(0);

  // ... there should be no problems shutting down this client ...
  BOOST_CHECK_NO_THROW(runner->resign());
  BOOST_CHECK_NO_THROW(runner.reset(nullptr));
}

/**
 * @test Verify that jb::etcd::leader_election_runner works when it
 * does not immediately win the election.
 */
BOOST_AUTO_TEST_CASE(leader_election_runner_must_wait) {
  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;
  completion_queue_type queue;
  prepare_mocks_for_not_initially_elected(queue);

  using namespace ::testing;
  // ... the runner class should also setup a Read() asynchronous operation
  // to receive the Watcher status updates.  The first time we will do
  // nothing other than save the operation ...
  using read_op_type = detail::read_op<etcdserverpb::WatchResponse>;
  std::shared_ptr<read_op_type> pending_read;
  EXPECT_CALL(*queue.interceptor().shared_mock, async_read(Truly([](auto op) {
    return op->name == "leader_election_participant/on_watch_create/read";
  }))).WillOnce(Invoke([r = std::ref(pending_read)](auto bop) {
    auto* op = dynamic_cast<read_op_type*>(bop.get());
    BOOST_REQUIRE(op != nullptr);

    r.get() = std::shared_ptr<read_op_type>(bop, op);

    // ... this time we do not immediately provide the callback, that
    // might happen later during the unit test ...
    //    DO NOT CALL THIS bop->callback(*bop, true);
  }));

  // ... the previous call happens only once, these many times ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_read(Truly([](auto op) {
    return op->name == "leader_election_participant/on_watch_read/read";
  }))).WillRepeatedly(Invoke([r = std::ref(pending_read)](auto bop) {
    auto* op = dynamic_cast<read_op_type*>(bop.get());
    BOOST_REQUIRE(op != nullptr);

    r.get() = std::shared_ptr<read_op_type>(bop, op);
    // avoid the callback until later in the unit test ...
    //    DO NOT CALL THIS bop->callback(*bop, true);
  }));

  bool elected = false;
  // ... we create this as a unique_ptr<> because there are things to
  // do right before and after the destructor ...
  auto runner = std::make_unique<runner_type>(
      queue, 0x123456UL, std::unique_ptr<etcdserverpb::KV::Stub>(),
      std::unique_ptr<etcdserverpb::Watch::Stub>(),
      std::string("test-election"), std::string("mocked-runner-a"),
      [&elected](std::future<bool>& src) { elected = src.get(); });

  // ... when it returns, the class should not be elected yet ...
  BOOST_CHECK_EQUAL(elected, false);

  // ... there should be a pending read for the watcher ...
  BOOST_REQUIRE(!!pending_read);

  // ... we use the captured asynchronous operation to send back a
  // response.  First we simulate a simple PUT event ...
  pending_read->response.set_created(true);
  pending_read->response.set_canceled(false);
  pending_read->response.set_watch_id(2000);
  {
    auto& ev = *pending_read->response.add_events();
    ev.set_type(mvccpb::Event::PUT);
    ev.mutable_kv()->set_key("test-election/A0A0A0");
  }

  // ... reset the pending read and send the callback, as if the
  // operation had completed ...
  auto pr = std::move(pending_read);
  BOOST_CHECK(!pending_read);
  pr->callback(*pr, true);

  // ... we expect another pending read because that kind of update is
  // ignored, first create another boring update ...
  BOOST_CHECK_EQUAL(elected, false);
  BOOST_REQUIRE(!!pending_read);

  pending_read->response.set_created(false);
  pending_read->response.set_canceled(false);
  pending_read->response.set_watch_id(2000);
  {
    auto& ev = *pending_read->response.add_events();
    ev.set_type(mvccpb::Event::PUT);
    ev.mutable_kv()->set_key("test-election/A0A0A0");
  }
  pr = std::move(pending_read);
  BOOST_CHECK(!pending_read);
  pr->callback(*pr, true);

  // ... same story, but now we create the update that makes things
  // interesting ...
  BOOST_CHECK_EQUAL(elected, false);
  BOOST_REQUIRE(!!pending_read);

  pending_read->response.set_created(false);
  pending_read->response.set_canceled(true);
  pending_read->response.set_watch_id(2000);
  {
    auto& ev = *pending_read->response.add_events();
    ev.set_type(mvccpb::Event::DELETE);
    ev.mutable_kv()->set_key("test-election/A0A0A0");
  }
  pr = std::move(pending_read);
  BOOST_CHECK(!pending_read);
  pr->callback(*pr, true);

  // ... we should have won the election ...
  BOOST_CHECK_EQUAL(elected, true);

  BOOST_CHECKPOINT("about to delete the runner");
  runner.reset(nullptr);
}

/**
 * @test Verify that jb::etcd::leader_election_runner works when it
 * does not immediately win the election.
 */
BOOST_AUTO_TEST_CASE(leader_election_runner_resign_during_campaign) {
  using namespace jb::etcd;
  completion_queue_type queue;
  prepare_mocks_for_not_initially_elected(queue);

  using namespace ::testing;
  // ... the runner class should also setup a Read() asynchronous operation
  // to receive the Watcher status updates.  The first time we will do
  // nothing other than save the operation ...
  using read_op_type = detail::read_op<etcdserverpb::WatchResponse>;
  std::shared_ptr<read_op_type> pending_read;
  EXPECT_CALL(*queue.interceptor().shared_mock, async_read(Truly([](auto op) {
    return op->name == "leader_election_participant/on_watch_create/read";
  }))).WillOnce(Invoke([r = std::ref(pending_read)](auto bop) {
    auto* op = dynamic_cast<read_op_type*>(bop.get());
    BOOST_REQUIRE(op != nullptr);

    r.get() = std::shared_ptr<read_op_type>(bop, op);

    // ... this time we do not immediately provide the callback, that
    // might happen later during the unit test ...
    //    DO NOT CALL THIS bop->callback(*bop, true);
  }));

  // ... the previous call happens only once, these many times ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_read(Truly([](auto op) {
    return op->name == "leader_election_participant/on_watch_read/read";
  }))).WillRepeatedly(Invoke([r = std::ref(pending_read)](auto bop) {
    auto* op = dynamic_cast<read_op_type*>(bop.get());
    BOOST_REQUIRE(op != nullptr);

    r.get() = std::shared_ptr<read_op_type>(bop, op);
    // avoid the callback until later in the unit test ...
    //    DO NOT CALL THIS bop->callback(*bop, true);
  }));

  bool elected = false;
  // ... we create this as a unique_ptr<> because there are things to
  // do right before and after the destructor ...
  auto runner = std::make_unique<runner_type>(
      queue, 0x123456UL, std::unique_ptr<etcdserverpb::KV::Stub>(),
      std::unique_ptr<etcdserverpb::Watch::Stub>(),
      std::string("test-election"), std::string("mocked-runner-a"),
      [&elected](std::future<bool>& src) { elected = src.get(); });

  // ... when it returns, the class should not be elected yet ...
  BOOST_CHECK_EQUAL(elected, false);

  // ... there should be a pending read for the watcher ...
  BOOST_REQUIRE(!!pending_read);

  // ... we use the captured asynchronous operation to send back a
  // response.  First we simulate a simple PUT event ...
  pending_read->response.set_created(true);
  pending_read->response.set_canceled(false);
  pending_read->response.set_watch_id(2000);
  {
    auto& ev = *pending_read->response.add_events();
    ev.set_type(mvccpb::Event::PUT);
    ev.mutable_kv()->set_key("test-election/A0A0A0");
  }
  // ... reset the pending read and send the callback, as if the
  // operation had completed ...
  auto pr = std::move(pending_read);
  BOOST_CHECK(!pending_read);
  pr->callback(*pr, true);
  // ... we expect another pending read because that kind of update is
  // ignored, first create another boring update ...
  BOOST_CHECK_EQUAL(elected, false);
  BOOST_REQUIRE(!!pending_read);

  // ... the resign() call would normally block waiting for the
  // pending read operation, but we can use a separate thread to
  // simulate that Read() finishing ...
  using namespace std::chrono_literals;
  std::thread t([pending_read]() {
    std::this_thread::sleep_for(10ms);
    pending_read->response.set_created(false);
    pending_read->response.set_canceled(true);
    pending_read->response.set_watch_id(2000);
    pending_read->callback(*pending_read, true);
  });

  // ... while the resign() call may block until the "asynchronous"
  // operation completes, it should complete eventually ...
  BOOST_CHECK_NO_THROW(runner->resign());

  t.join();

  BOOST_CHECKPOINT("about to delete the runner");
  BOOST_CHECK_NO_THROW(runner.reset(nullptr));
}

/**
 * @test Verify that jb::etcd::leader_election_runner works if
 * preamble() raises an exception.
 */
BOOST_AUTO_TEST_CASE(leader_election_runner_preamble_exception) {
  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;
  completion_queue_type queue;
  prepare_mocks_common(queue);

  using namespace ::testing;
  // ... cause an exception by canceling the asynchronous operation to
  // set initial value ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(Truly([](auto op) {
    return op->name == "leader_election/commit/create_node";
  }))).WillOnce(Invoke([](auto bop) {
    // ... false indicates the operation was canceled ...
    bop->callback(*bop, false);
  }));

  // ... the test itself is easy ...
  bool elected = false;
  std::unique_ptr<runner_type> runner;
  BOOST_CHECK_THROW(
      runner = std::make_unique<runner_type>(
          queue, 0x123456UL, std::unique_ptr<etcdserverpb::KV::Stub>(),
          std::unique_ptr<etcdserverpb::Watch::Stub>(),
          std::string("test-election"), std::string("mocked-runner-a"),
          [&elected](std::future<bool>& src) { elected = src.get(); }),
      std::exception);
  BOOST_CHECK_EQUAL(elected, false);
}

/**
 * @test Verify that jb::etcd::leader_election_runner works if the
 * node already exists and its creation fails.
 */
BOOST_AUTO_TEST_CASE(leader_election_runner_preamble_create_node_fails) {
  // Create a null lease object, we do not need (or want) a real
  // connection for mocked operations ...
  std::shared_ptr<etcdserverpb::Lease::Stub> lease;

  using namespace jb::etcd;
  completion_queue_type queue;
  prepare_mocks_common(queue);

  using namespace ::testing;
  // ... the runner will try to create a node for the participant using
  // async_rpc, provide an initial (logical, not RPC) failure ..
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(Truly([](auto op) {
    return op->name == "leader_election/commit/create_node";
  }))).WillOnce(Invoke([](auto bop) {
    using op_type = jb::etcd::detail::async_op<
        etcdserverpb::TxnRequest, etcdserverpb::TxnResponse>;
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
    op->response.set_succeeded(false);
    op->response.mutable_header()->set_revision(4000);
    auto& resp = *op->response.add_responses();
    auto& kv = *resp.mutable_response_range()->add_kvs();
    kv.set_create_revision(2000UL); // really old ...
    kv.set_value("mocked-runner-a");
    // ... and to not forget the callback ...
    bop->callback(*bop, true);
  }));

  prepare_mocks_for_initially_elected_post_node(queue, 2000);

  // ... the test itself is easy ...
  bool elected = false;
  std::unique_ptr<runner_type> runner = std::make_unique<runner_type>(
      queue, 0x123456UL, std::unique_ptr<etcdserverpb::KV::Stub>(),
      std::unique_ptr<etcdserverpb::Watch::Stub>(),
      std::string("test-election"), std::string("mocked-runner-a"),
      [&elected](std::future<bool>& src) { elected = src.get(); });
  BOOST_CHECK_EQUAL(elected, true);
  BOOST_CHECK_EQUAL(runner->participant_revision(), 2000);
  BOOST_CHECK_NO_THROW(runner.reset(nullptr));
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
}

void prepare_mocks_for_initially_elected_post_node(
    completion_queue_type& queue, int revision) {
  using namespace ::testing;
  // ... shortly after creating a node, the class will request the
  // range of other nodes with the same prefix ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(Truly([](auto op) {
    return op->name == "leader_election_participant/campaign/range";
  }))).WillOnce(Invoke([revision](auto bop) {
    using op_type = jb::etcd::detail::async_op<
        etcdserverpb::RangeRequest, etcdserverpb::RangeResponse>;
    auto* op = dynamic_cast<op_type*>(bop.get());
    BOOST_REQUIRE(op != nullptr);
    // ... verify the request is what we expect ...
    BOOST_REQUIRE_EQUAL(op->request.key(), "test-election/");
    BOOST_REQUIRE_EQUAL(op->request.range_end(), "test-election0");
    BOOST_CHECK_EQUAL(op->request.max_create_revision(), revision - 1);
    BOOST_CHECK_EQUAL(
        op->request.sort_order(), etcdserverpb::RangeRequest::DESCEND);
    BOOST_CHECK_EQUAL(
        op->request.sort_target(), etcdserverpb::RangeRequest::CREATE);
    BOOST_CHECK_EQUAL(op->request.limit(), 1);

    // ... in this test we just assume everything is simple, just
    // provide an empty response ...

    // ... and to not forget the callback ...
    bop->callback(*bop, true);
  }));
}

void prepare_mocks_for_initially_elected(completion_queue_type& queue) {
  // The sequences of queries for a leader election participant is
  // rather long, but it goes like this ...
  using namespace ::testing;
  prepare_mocks_common(queue);

  // ... the class will try to create a node for the participant using
  // async_rpc, provide a good response for it ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(Truly([](auto op) {
    return op->name == "leader_election/commit/create_node";
  }))).WillOnce(Invoke([](auto bop) {
    using op_type = jb::etcd::detail::async_op<
        etcdserverpb::TxnRequest, etcdserverpb::TxnResponse>;
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
    op->response.mutable_header()->set_revision(3000);
    // ... and to not forget the callback ...
    bop->callback(*bop, true);
  }));

  prepare_mocks_for_initially_elected_post_node(queue, 3000);
}

void prepare_mocks_for_not_initially_elected(completion_queue_type& queue) {
  using namespace ::testing;
  prepare_mocks_common(queue);

  // ... the class will try to create a node for the participant using
  // async_rpc, provide a good response for it ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_rpc(Truly([](auto op) {
    return op->name == "leader_election/commit/create_node";
  }))).WillOnce(Invoke([](auto bop) {
    using op_type = jb::etcd::detail::async_op<
        etcdserverpb::TxnRequest, etcdserverpb::TxnResponse>;
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
    using op_type = jb::etcd::detail::async_op<
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

    // ... in this test we return a list of 1 element with an older
    // creation time ...
    auto& kv = *op->response.add_kvs();
    kv.set_key("test-election/A0A0A0");
    kv.set_value("beat you to it!");
    op->response.mutable_header()->set_revision(1000UL);

    // ... and to not forget the callback ...
    bop->callback(*bop, true);
  }));

  // ... because the range request gets more than 0 values, the
  // leader_election_runner should set up a watcher on it ...
  EXPECT_CALL(*queue.interceptor().shared_mock, async_write(Truly([](auto op) {
    return op->name == "leader_election_participant/on_range_request/watch";
  }))).WillOnce(Invoke([](auto bop) {
    using op_type = jb::etcd::detail::write_op<etcdserverpb::WatchRequest>;
    auto* op = dynamic_cast<op_type*>(bop.get());
    BOOST_REQUIRE(op != nullptr);
    // ... verify the request is what we expect ...
    BOOST_REQUIRE(op->request.has_create_request());
    BOOST_CHECK_EQUAL(
        op->request.create_request().key(), "test-election/A0A0A0");
    BOOST_CHECK_EQUAL(op->request.create_request().start_revision(), 999);

    // ... and to not forget the callback ...
    bop->callback(*bop, true);
  }));
}

} // anonymous namespace
