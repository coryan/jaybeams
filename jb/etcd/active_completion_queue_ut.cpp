#include "jb/etcd/active_completion_queue.hpp"

#include <boost/test/unit_test.hpp>
#include <atomic>
#include <thread>

/**
 * @test Verify that jb::etcd::active_completion_queue works as expected.
 */
BOOST_AUTO_TEST_CASE(active_completion_queue_basic) {
  auto shq = std::make_shared<jb::etcd::active_completion_queue>();
  BOOST_CHECK_NO_THROW(shq.reset());

  BOOST_CHECK_NO_THROW(jb::etcd::active_completion_queue());

  {
    jb::etcd::active_completion_queue orig;
    jb::etcd::active_completion_queue copy(std::move(orig));
    BOOST_CHECK((grpc::CompletionQueue*)orig == nullptr);
    BOOST_CHECK((grpc::CompletionQueue*)copy != nullptr);
  }

  {
    jb::etcd::active_completion_queue orig;
    BOOST_CHECK((grpc::CompletionQueue*)orig != nullptr);
    jb::etcd::active_completion_queue copy;
    BOOST_CHECK((grpc::CompletionQueue*)copy != nullptr);

    copy = std::move(orig);
    BOOST_CHECK((grpc::CompletionQueue*)orig == nullptr);
    BOOST_CHECK((grpc::CompletionQueue*)copy != nullptr);
  }
  
  auto cq = std::make_shared<jb::etcd::completion_queue>();
  std::thread t([cq]() { cq->run(); });
  BOOST_CHECK(t.joinable());

  {
    jb::etcd::active_completion_queue owner(std::move(cq), std::move(t));
    BOOST_CHECK((grpc::CompletionQueue*)owner != nullptr);
    BOOST_CHECK(not t.joinable());
  }
}
