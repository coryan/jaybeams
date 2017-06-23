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
  auto timer = queue.make_relative_timer(
      std::chrono::milliseconds(10), [&cnt](auto op) { ++cnt; });
  std::thread t([&queue]() { queue.run(); });

  for (int i = 0; i != 100 and cnt.load() == 0; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  BOOST_CHECK_EQUAL(cnt.load(), 1);
  queue.shutdown();
  t.join();
}
