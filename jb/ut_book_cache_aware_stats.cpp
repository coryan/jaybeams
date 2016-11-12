#include <jb/book_cache_aware_stats.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that jb::book_cache_aware_stats works as expected.
 */
BOOST_AUTO_TEST_CASE(book_cache_aware_stats_simple) {
  jb::book_cache_aware_stats::config cfg;
  jb::book_cache_aware_stats stats(cfg);

  stats.sample(1, 10);
  stats.sample(2, 20);
  stats.sample(3, 30);
  stats.sample(4, 40);
  stats.sample(5, 50);

  static_assert(
      std::is_same<jb::itch5::tick_t, jb::tick_t>::value,
      "Mismatched definition of "
      "jb::itch5::tick_t and jb::tick_t");
  static_assert(
      std::is_same<jb::itch5::level_t, jb::level_t>::value,
      "Mismatched definition of "
      "jb::itch5::level_t and jb::level_t");
}

/**
 * @test Test jb::book_cache_aware_stats csv output.
 */
BOOST_AUTO_TEST_CASE(book_cache_aware_stats_print_csv) {
  jb::book_cache_aware_stats::config cfg;
  jb::book_cache_aware_stats stats(cfg);
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
  stats.sample(5, 50);
  stats.sample(2, 20);
  stats.sample(3, 30);
  stats.sample(4, 40);

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
  stats.sample(1, 10);
  body.str("");
  stats.print_csv("testing", body);
  BOOST_CHECK_EQUAL(body.str().substr(0, 10), std::string("testing,5,"));
  BOOST_CHECK_EQUAL(body.str().substr(10, 2), std::string("1,"));

  BOOST_TEST_MESSAGE("CSV Output for inspection: \n" << h << b);
}

/**
 * @test Verify that jb::book_cache_aware_stats::config works as expected.
 */
BOOST_AUTO_TEST_CASE(book_cache_aware_stats_config_simple) {
  typedef jb::book_cache_aware_stats::config config;

  BOOST_CHECK_NO_THROW(config().validate());
  BOOST_CHECK_THROW(config().max_ticks(0).validate(), jb::usage);
  BOOST_CHECK_THROW(config().max_levels(0).validate(), jb::usage);
}
