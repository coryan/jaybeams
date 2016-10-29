#include <jb/book_depth_statistics.hpp>

#include <boost/test/unit_test.hpp>

constexpr bool eqTypes() {
  return std::is_same<jb::itch5::book_depth_t, jb::book_depth_t>::value;}

/**
 * @test Verify that jb::book_depth_statistics works as expected.
 */
BOOST_AUTO_TEST_CASE(book_depth_statistics_simple) {
  jb::book_depth_statistics::config cfg;
  jb::book_depth_statistics stats(cfg);
  
  stats.sample(1);
  stats.sample(2);
  stats.sample(3);
  stats.sample(4);
  stats.sample(5);

  BOOST_STATIC_ASSERT_MSG(eqTypes(),
      "inconsistent types jb:: and jb::itch5:: book_depth_t");
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
  // save number of headers...
  int nheaders = std::count(h.begin(), h.end(), ',');

  std::ostringstream body;

  // testing header format
  stats.print_csv("testing", body);
  BOOST_CHECK_EQUAL(body.str().substr(0, 10), std::string("testing,0,"));
  std::string b = body.str();
  int nfields = std::count(b.begin(), b.end(), ',');
  BOOST_CHECK_EQUAL(nfields, nheaders);

  // 4 samples, book_depth {2..5}
  stats.sample(5);
  stats.sample(2);
  stats.sample(3);
  stats.sample(4);

  // check 4 samples... 
  body.str("");
  stats.print_csv("testing", body);
  BOOST_CHECK_EQUAL(body.str().substr(0, 10), std::string("testing,4,"));
  // ... minBookDepth = 2
  BOOST_CHECK_EQUAL(body.str().substr(10, 2), std::string("2,"));
  // ... number of fields = to #headers
  b = body.str();
  nfields = std::count(b.begin(), b.end(), ',');
  BOOST_CHECK_EQUAL(nfields, nheaders);
  
  // add one more sample (# 5), book_depth now {1..5}
  stats.sample(1);
  body.str("");
  stats.print_csv("testing", body);
  BOOST_CHECK_EQUAL(body.str().substr(0, 10), std::string("testing,5,"));
  BOOST_CHECK_EQUAL(body.str().substr(10, 2), std::string("1,"));

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
