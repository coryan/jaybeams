#include <jb/book_depth_statistics.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::book_depth_statistics works as expected.
 */
BOOST_AUTO_TEST_CASE(book_depth_statistics_simple) {
  jb::book_depth_statistics::config cfg;
  jb::book_depth_statistics stats(cfg);
  
  stats.sample(std::chrono::seconds(1),1);
  stats.sample(std::chrono::seconds(1) + std::chrono::microseconds(1),2);
  stats.sample(std::chrono::seconds(1) + std::chrono::microseconds(2),3);
  stats.sample(std::chrono::seconds(1) + std::chrono::microseconds(3),4);
  stats.sample(std::chrono::seconds(1) + std::chrono::microseconds(4),5);
  
}

/**
 * @test Test jb::book_depth_statistics csv output.
 */
BOOST_AUTO_TEST_CASE(book_depth_statistics_print_csv) {
  jb::book_depth_statistics::config cfg;
  jb::book_depth_statistics stats(cfg);
  std::ostringstream header;
  stats.print_csv_header(header);
  BOOST_CHECK_EQUAL(header.str().substr(0, 5), std::string("Name,"));

  std::string h = header.str();
  int nheaders = std::count(h.begin(), h.end(), ',');

  std::ostringstream body;
  stats.print_csv("testing", body);
  BOOST_CHECK_EQUAL(body.str().substr(0, 10), std::string("testing,0,"));
  std::string b = body.str();
  int nfields = std::count(b.begin(), b.end(), ',');
  BOOST_CHECK_EQUAL(nfields, nheaders);

  stats.sample(std::chrono::seconds(1) + std::chrono::microseconds(1),5);
  stats.sample(std::chrono::seconds(1) + std::chrono::microseconds(2),2);
  stats.sample(std::chrono::seconds(1) + std::chrono::microseconds(3),3);
  stats.sample(std::chrono::seconds(1) + std::chrono::microseconds(4),4);

  body.str("");
  stats.print_csv("testing", body);
  BOOST_CHECK_EQUAL(body.str().substr(0, 10), std::string("testing,2,"));
  b = body.str();
  nfields = std::count(b.begin(), b.end(), ',');
  BOOST_CHECK_EQUAL(nfields, nheaders);

  stats.sample(std::chrono::seconds(1) + std::chrono::microseconds(5),1);
  body.str("");
  stats.print_csv("testing", body);
  BOOST_CHECK_EQUAL(body.str().substr(0, 10), std::string("testing,1,"));

  BOOST_TEST_MESSAGE("CSV Output for inspection: \n" << h << b);
}

/**
 * @test Verify that jb::book_depth_statistics::config works as expected.
 */
BOOST_AUTO_TEST_CASE(book_depth_statististics_config_simple) {
  typedef jb::book_depth_statistics::config config;

  BOOST_CHECK_NO_THROW(config().validate());
  BOOST_CHECK_THROW(
    config().max_book_depth(0).validate(), jb::usage);
}
