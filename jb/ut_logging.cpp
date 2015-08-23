#include <jb/log.hpp>
#include <jb/as_hhmmss.hpp>

#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/test/unit_test.hpp>
#include <sstream>

/**
 * @test Verify that basic logging functions work as expected.
 */
BOOST_AUTO_TEST_CASE(logging_basic) {
  jb::log::init(
      jb::log::config()
      .minimum_severity(jb::severity_level::debug)
      .enable_file_logging(true)
      .logfile_basename("ut_logging"));

  JB_LOG(trace) << "tracing tracing tracing";
  for (int i = 0; i != 30000; ++i) {
    JB_LOG(debug) << "i=" << i;
    jb::log::next_tid();
  }
  JB_LOG(info) << "testing my logger (" << 1 << ")";
  JB_LOG(warning) << "testing warnings in my logger";
  JB_LOG(error) << "here is an error: " << jb::severity_level::warning;

  std::string foo("more complex expr test");
  int x = 1;
  float y = 2.0;

  auto expensive = [](int n) {
    int s = 0;
    for(int i = 0; i != n; ++i) {
      s += n;
    }
    return s;
  };

  jb::log::next_tid();
  JB_LOG(trace) << "this never gets to run, and the optimizer "
                << "should remove it" << expensive(20);
  JB_LOG(debug) << "this does not run, but the compiler will keep..."
                << expensive(1000);
  JB_LOG(notice) << "L3 x=" << x << ", foo=" << foo << ", y=" << y;
  JB_LOG(warning) << "L3 this";

  JB_LOG(notice) << "x=" << x << ", foo=" << foo << " y=" << y;
  JB_LOG(debug) << "x=" << x << ", foo=" << foo << " y=" << y;
  JB_LOG(error) <<  "x=" << x << ", foo=" << foo << " y=" << y;

  std::ostringstream os;
  auto core = boost::log::core::get();
  auto backend = boost::make_shared<boost::log::sinks::text_ostream_backend>();
  backend->add_stream(
      boost::shared_ptr< std::ostream >(&os, [](void const*){} ));
  backend->auto_flush(true);
  typedef boost::log::sinks::synchronous_sink<
    boost::log::sinks::text_ostream_backend> sink_t;
  auto sink = boost::make_shared<sink_t>(backend);
  core->add_sink(sink);

  JB_LOG(alert) << "this is a log line";

  BOOST_CHECK_EQUAL(os.str(), "this is a log line\n");

  core->remove_sink(sink);

  jb::log::next_tid();
  JB_LOG(info) << "more logging after removing the sunk...";
}
