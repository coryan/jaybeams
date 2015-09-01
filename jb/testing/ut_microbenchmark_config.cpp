#include <jb/testing/microbenchmark_config.hpp>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(microbenchmark_config_default) {
  jb::testing::microbenchmark_config config;

  BOOST_CHECK_GT(config.iterations(), 0);
}

BOOST_AUTO_TEST_CASE(microbenchmark_config_modify) {
  jb::testing::microbenchmark_config config;

  config.iterations(10)
      .warmup_iterations(11)
      ;
  BOOST_CHECK_EQUAL(config.iterations(), 10);
  BOOST_CHECK_EQUAL(config.warmup_iterations(), 11);
}

BOOST_AUTO_TEST_CASE(microbenchmark_config_cmdline) {
  char argv0[] = "a/b/c";
  char argv1[] = "--iterations=10";
  char argv2[] = "--warmup-iterations=11";
  char *argv[] = {argv0, argv1, argv2};
  int argc = sizeof(argv) / sizeof(argv[0]);

  jb::testing::microbenchmark_config config;
  config.process_cmdline(argc, argv);

  BOOST_CHECK_EQUAL(config.iterations(), 10);
  BOOST_CHECK_EQUAL(config.warmup_iterations(), 11);
}
