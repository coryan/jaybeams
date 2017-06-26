#include "jb/etcd/completion_queue.hpp"

#include <boost/test/unit_test.hpp>
#include <atomic>
#include <thread>

/**
 * @test Verify that one can create, run, and stop a completion queue.
 */
BOOST_AUTO_TEST_CASE(completion_queue_basic) {
  jb::etcd::completion_queue queue;

  std::atomic<int> cnt(0);
  std::atomic<int> cxl(0);
  auto functor = [&cnt, &cxl](auto const& op, bool ok) {
    if (not ok) {
      ++cxl;
    } else {
      ++cnt;
    }
  };

  auto canceled = queue.make_relative_timer(
      std::chrono::milliseconds(10), "test-canceled", functor);
  canceled->cancel();
  auto timer = queue.make_relative_timer(
      std::chrono::milliseconds(10), "test-timer", functor);
  std::thread t([&queue]() { queue.run(); });

  for (int i = 0; i != 100 and cnt.load() == 0; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  BOOST_CHECK_EQUAL(cnt.load(), 1);
  BOOST_CHECK_EQUAL(cxl.load(), 1);

  queue.shutdown();
  t.join();
}

/**
 * @test Make sure jb::etcd::completion_queue handles errors gracefully.
 */
BOOST_AUTO_TEST_CASE(completion_queue_error) {
  using namespace std::chrono_literals;

  jb::etcd::completion_queue queue;
  std::thread t([&queue]() { queue.run(); });

  // ... manually create a timer with a null tag, that requires going
  // around the API ...
  grpc::CompletionQueue* cq = (grpc::CompletionQueue*)queue;

  std::atomic<int> cnt(0);
  auto op = queue.make_relative_timer(
      20ms, "alarm-after", [&cnt](auto const& op, bool ok) { ++cnt; });
  // ... also set an earlier alarm with an unused nullptr tag ...
  grpc::Alarm al1(cq, std::chrono::system_clock::now() + 10ms, nullptr);
  // ... and an alarm with a tag the queue does not know about ...
  grpc::Alarm al2(cq, std::chrono::system_clock::now() + 10ms, (void*)&cnt);

  for (int i = 0; i != 100 and cnt.load() == 0; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  BOOST_CHECK_EQUAL(cnt.load(), 1);

  queue.shutdown();
  t.join();
}
