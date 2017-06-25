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
