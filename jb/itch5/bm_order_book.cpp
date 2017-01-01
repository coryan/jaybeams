/**
 * @file
 *
 * This is a benchmark for jb::itch5::array_based_order_book and
 * jb::itch5::map_based_order_book.  It computes a valid stream of
 * book operations, and runs the operations over the book type
 * multiple times to measure its performance and variability.
 *
 * Some effort is made to make sure the stream of book operations is
 * statistically similar to the observed behaviour of the ITCH-5.0
 * feed.
 *
 * To make the test reproduceable, the user can pass in the seed to
 * the PRNGs used to create the stream of book operations.
 *
 * The program uses the jb::microbenchmark<> class, taking advantage
 * of common features such as command-line configurable number of
 * iterations, scheduling attributes, warmup cycles, etc.
 */
#include <jb/itch5/array_based_order_book.hpp>
#include <jb/itch5/map_based_order_book.hpp>
#include <jb/itch5/price_levels.hpp>
#include <jb/testing/compile_info.hpp>
#include <jb/testing/microbenchmark.hpp>
#include <jb/log.hpp>

#include <random>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

/**
 * Define types and functions used in this program.
 */
namespace {
/// Configuration parameters for the microbenchmark fixture
class fixture_config : public jb::config_object {
public:
  fixture_config();
  config_object_constructors(fixture_config);

  void validate() const override;

  jb::config_attribute<fixture_config, int> p25;
  jb::config_attribute<fixture_config, int> p50;
  jb::config_attribute<fixture_config, int> p75;
  jb::config_attribute<fixture_config, int> p90;
  jb::config_attribute<fixture_config, int> p99;
  jb::config_attribute<fixture_config, int> p999;
  jb::config_attribute<fixture_config, int> p100;
};

/// Configuration parameters for bm_order_book
class config : public jb::config_object {
public:
  config();
  config_object_constructors(config);

  void validate() const override;

  using array_config = typename jb::itch5::array_based_order_book::config;
  using map_config = typename jb::itch5::map_based_order_book::config;

  jb::config_attribute<config, jb::log::config> log;
  jb::config_attribute<config, jb::testing::microbenchmark_config>
      microbenchmark;
  jb::config_attribute<config, array_config> array_book;
  jb::config_attribute<config, map_config> map_book;
  jb::config_attribute<config, fixture_config> fixture;
  jb::config_attribute<config, unsigned int> seed;
};

/// Run the benchmark for a specific book type
template <typename book_side, typename book_config>
class fixture {
public:
  /// The default number of iterations for the benchmark
  static int constexpr default_size = 100000;

  /// Default constructor, delegate on the constructor given the
  /// number of iterations
  fixture(fixture_config const& cfg, book_config const& bkcfg, int seed)
      : fixture(default_size, cfg, bkcfg, seed) {
  }

  /**
   * Construct a new test fixture.
   *
   * @param size the number of events (order add / mod / delete) to
   * simulate.
   */
  fixture(
      int size, fixture_config const& cfg, book_config const& bkcfg,
      unsigned int seed)
      : bkcfg_(bkcfg) {
    // The benchmark prepares a list of @a size operations to execute
    // in the run() function.  We do this in the constructor because
    // we want to measure the time to execute the operations, not the
    // time to generate them ...
    std::vector<operation> operations;
    operations.reserve(size);

    // ... the operations are generated at random, we use one of the
    // standard PRNG in the library.  Notice that the seed is a
    // parameter into the benchmark, so it can be executed multiple
    // times with the same sequence of operations ...
    // BTW, I am aware (see [1]) of the deficiencies in initializing a
    // Mersenne-Twister with only 32 bits of seed, I am not looking
    // for a statistically unbiased, nor a cryptographically strong
    // PRNG.  We just want something that explores more of the test
    // space than normal, and can also be easily controlled from the
    // command line.
    // [1]: http://www.pcg-random.org/blog/
    std::mt19937_64 generator(seed);

    // ... use the configuration parameters to create a "depth
    // distribution".  The default values are chosen from
    // realistic market data, so the benchmark should generate data
    // that is statistically similar to what the market shows ...
    std::vector<int> boundaries(
        {0, cfg.p25(), cfg.p50(), cfg.p75(), cfg.p90(), cfg.p99(), cfg.p999(),
         cfg.p100()});
    std::vector<double> weights(
        {0, 0.25, 0.25, 0.25, 0.15, 0.09, 0.009, 0.001});
    JB_ASSERT_THROW(boundaries.size() == weights.size());
    std::piecewise_linear_distribution<> ddis(
        boundaries.begin(), boundaries.end(), weights.begin());

    // ... we also randomly pick whenther the new operation is better
    // or worse than the current inside ...
    std::uniform_int_distribution<> bdis(0, 1);

    // ... for the size, we use a uniform distribution in the
    // [-1000,1000] range, though we have to trim the results based on
    // what is actually in the book ...
    std::uniform_int_distribution<> sdis(-1000, 1000);

    // ... save the maximum possible price level ...
    using jb::itch5::price4_t;
    int const max_level = jb::itch5::price_levels(
        price4_t(0), jb::itch5::max_price_field_value<price4_t>());

    // ... we need a function to convert price levels to prices, the
    // function depends on whether the book is ascending by price
    // (BUY) or descending by price (SELL).  Writing this function in
    // terms of price levels and then converting to prices makes the
    // code easier to read ...
    std::function<price4_t(int)> level2price;
    book_side tmp(bkcfg_);
    if (tmp.is_ascending()) {
      level2price = [](int level) {
        return jb::itch5::level_to_price<price4_t>(level);
      };
    } else {
      level2price = [max_level](int level) {
        return jb::itch5::level_to_price<price4_t>(max_level - level);
      };
    }

    // ... keep track of the contents in a simulated book, indexed by
    // price level ...
    std::map<int, int> book;

    // ... start the book with a large order at 100000 levels from the
    // base level, this is just so the initial operations do not
    // create a lot of random noise ...
    int const initial_level = 100000;
    int const initial_qty = 5000;
    operations.push_back({level2price(initial_level), initial_qty});
    book[initial_level] = initial_qty;

    for (int i = 1; i != size; ++i) {
      // ... find out what is the current best level, or use
      // initial_level if it is not available ...
      int best_level;
      if (not book.empty()) {
        best_level = book.rbegin()->first;
      } else {
        best_level = initial_level;
      }
      // ... generate a new level to operate at, and a qty to modify
      // ...
      int level;
      if (bdis(generator)) {
        level = best_level - static_cast<int>(ddis(generator));
      } else {
        level = best_level + static_cast<int>(ddis(generator));
      }
      int qty = sdis(generator);
      // ... normalize the level to the valid range ...
      if (level <= 0) {
        level = 1;
      } else if (level >= max_level) {
        level = max_level - 1;
      }
      // ... normalize the qty too ...
      if (qty == 0) {
        qty = 100;
      } else if (qty < 0) {
        auto f = book.find(level);
        if (f == book.end()) {
          qty = -qty;
        } else if (f->second + qty < 0) {
          qty = -f->second;
        }
      }
      // ... now insert the operation into the array, and record it in
      // the book ...
      book[level] += qty;
      if (book[level] == 0) {
        book.erase(level);
      }
      operations.push_back({level2price(level), qty});
    }
    operations_ = std::move(operations);
  }

  /// Run an iteration of the test ...
  void run() {
    // ... creating a book in each iteration is less than ideal, but
    // it is the only way to make sure the operations are applied to a
    // well-known state ...
    book_side book(bkcfg_);
    // ... iterate over the operations and pass them to the book ...
    for (auto const& op : operations_) {
      if (op.delta < 0) {
        book.reduce_order(op.px, -op.delta);
      } else {
        book.add_order(op.px, op.delta);
      }
    }
  }

private:
  /// The configuration for the book side
  book_config bkcfg_;

  /// A simple representation for book operations.
  struct operation {
    /// The price to modify
    jb::itch5::price4_t px;
    /// The quantity, negative values indicate the operation is a
    /// remove_order(), positive that it is an add_order() operation.
    int delta;
  };

  /// The sequence of operations to apply...
  std::vector<operation> operations_;
};

/**
 * Run the benchmark for a specific book type
 *
 * @param cfg the configuration for the benchmark
 * @param book_cfg the configuration for the specific book type
 *
 * @tparam book_type the type of book to use in the benchmark
 * @tparam book_type_config the configuration class for @a book_type
 */
template <typename book_side, typename book_type_config>
void run_benchmark(config const& cfg, book_type_config const& book_cfg) {
  // The benchmark creates a (pseudo-)random sequence of operations
  // and measures their performance.  The same set of operations is
  // tested over and over, by providing the same seed to the PRNG in
  // the test.  The user can even use the same seed in two runs of the
  // benchmark by providing a non-zero value as a command-line
  // argument.
  auto seed = cfg.seed();
  if (seed == 0) {
    // ... by default we initialize the seed with whatever system
    // entry source is available in the standard C++ library.
    seed = std::random_device()();
  }
  JB_LOG(info) << "Running benchmark for " << cfg.microbenchmark().test_case()
               << " with SEED=" << seed;
  using benchmark =
      jb::testing::microbenchmark<fixture<book_side, book_type_config>>;
  benchmark bm(cfg.microbenchmark());
  auto r = bm.run(cfg.fixture(), book_cfg, seed);

  typename benchmark::summary s(r);
  JB_LOG(info) << cfg.microbenchmark().test_case() << " summary " << s;
  if (cfg.microbenchmark().verbose()) {
    bm.write_results(std::cout, r);
  }
}
} // anonymous namespace

int main(int argc, char* argv[]) try {
  config cfg;
  cfg.process_cmdline(argc, argv);

  // initialize the logging framework ...
  jb::log::init(cfg.log());
  if (cfg.microbenchmark().verbose()) {
    JB_LOG(info) << "Configuration for test\n" << cfg;
    JB_LOG(info) << "Compile-time configuration:"
                 << "\nuname:          " << jb::testing::uname_a
                 << "\ncompiler:       " << jb::testing::compiler
                 << "\ncompiler flags: " << jb::testing::compiler_flags
                 << "\nlinker:         " << jb::testing::linker << "\n";
  }

  using namespace jb::itch5;
  auto test_case = cfg.microbenchmark().test_case();
  if (test_case == "array:buy") {
    run_benchmark<array_based_order_book::buys_t>(cfg, cfg.array_book());
  } else if (test_case == "array:sell") {
    run_benchmark<array_based_order_book::sells_t>(cfg, cfg.array_book());
  } else if (test_case == "map:buy") {
    run_benchmark<map_based_order_book::buys_t>(cfg, cfg.map_book());
  } else if (test_case == "map:sell") {
    run_benchmark<map_based_order_book::sells_t>(cfg, cfg.map_book());
  } else {
    // ... it is tempting to move this code to the validate() member
    // function, but then we have to repeat the valid configurations
    // twice in the code ...
    std::ostringstream os;
    os << "Unknown test case (" << test_case << ")" << std::endl;
    os << " --microbenchmark.test-case must be one of"
       << ": array:buy, array:sell"
       << ", map:buy, map:sell" << std::endl;
    throw jb::usage(os.str(), 1);
  }

  return 0;
} catch (jb::usage const& ex) {
  std::cerr << "usage: " << ex.what() << std::endl;
  return ex.exit_status();
} catch (std::exception const& ex) {
  std::cerr << "standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "unknown exception raised" << std::endl;
  return 1;
}

namespace {
namespace defaults {
#ifndef JB_ITCH5_DEFAULT_bm_array_based_order_book_test_case
#define JB_ITCH5_DEFAULT_bm_array_based_order_book_test_case "array:buy"
#endif // JB_ITCH5_DEFAULT_bm_array_based_order_book_test_case

#ifndef JB_ITCH5_DEFAULT_bm_array_based_order_book_p25
#define JB_ITCH5_DEFAULT_bm_array_based_order_book_p25 0
#endif // JB_ITCH5_DEFAULT_bm_array_based_order_book_p25

#ifndef JB_ITCH5_DEFAULT_bm_array_based_order_book_p50
#define JB_ITCH5_DEFAULT_bm_array_based_order_book_p50 1
#endif // JB_ITCH5_DEFAULT_bm_array_based_order_book_p50

#ifndef JB_ITCH5_DEFAULT_bm_array_based_order_book_p75
#define JB_ITCH5_DEFAULT_bm_array_based_order_book_p75 6
#endif // JB_ITCH5_DEFAULT_bm_array_based_order_book_p75

#ifndef JB_ITCH5_DEFAULT_bm_array_based_order_book_p90
#define JB_ITCH5_DEFAULT_bm_array_based_order_book_p90 14
#endif // JB_ITCH5_DEFAULT_bm_array_based_order_book_p90

#ifndef JB_ITCH5_DEFAULT_bm_array_based_order_book_p99
#define JB_ITCH5_DEFAULT_bm_array_based_order_book_p99 203
#endif // JB_ITCH5_DEFAULT_bm_array_based_order_book_p99

#ifndef JB_ITCH5_DEFAULT_bm_array_based_order_book_p999
#define JB_ITCH5_DEFAULT_bm_array_based_order_book_p999 2135
#endif // JB_ITCH5_DEFAULT_bm_array_based_order_book_p999

#ifndef JB_ITCH5_DEFAULT_bm_array_based_order_book_p100
#define JB_ITCH5_DEFAULT_bm_array_based_order_book_p100 20000000
#endif // JB_ITCH5_DEFAULT_bm_array_based_order_book_p100

std::string const test_case =
    JB_ITCH5_DEFAULT_bm_array_based_order_book_test_case;
int constexpr p25 = JB_ITCH5_DEFAULT_bm_array_based_order_book_p25;
int constexpr p50 = JB_ITCH5_DEFAULT_bm_array_based_order_book_p50;
int constexpr p75 = JB_ITCH5_DEFAULT_bm_array_based_order_book_p75;
int constexpr p90 = JB_ITCH5_DEFAULT_bm_array_based_order_book_p90;
int constexpr p99 = JB_ITCH5_DEFAULT_bm_array_based_order_book_p99;
int constexpr p999 = JB_ITCH5_DEFAULT_bm_array_based_order_book_p999;
int constexpr p100 = JB_ITCH5_DEFAULT_bm_array_based_order_book_p100;

} // namespace defaults

fixture_config::fixture_config()
    : p25(desc("p25").help(
              "Define the maximum depth of 25% of the events. "
              "The benchmark generates random book changes, with the depth of "
              "these changes controled by this argument (and similar ones), "
              "the default values are chosen to match the observed behavior "
              "in real market feeds."),
          this, defaults::p25)
    , p50(desc("p50").help(
              "Define the maximum depth of 50% of the events. "
              "The benchmark generates random book changes, with the depth of "
              "these changes controled by this argument (and similar ones), "
              "the default values are chosen to match the observed behavior "
              "in real market feeds."),
          this, defaults::p50)
    , p75(desc("p75").help(
              "Define the maximum depth of 2\75% of the events. "
              "The benchmark generates random book changes, with the depth of "
              "these changes controled by this argument (and similar ones), "
              "the default values are chosen to match the observed behavior "
              "in real market feeds."),
          this, defaults::p75)
    , p90(desc("p90").help(
              "Define the maximum depth of 90% of the events. "
              "The benchmark generates random book changes, with the depth of "
              "these changes controled by this argument (and similar ones), "
              "the default values are chosen to match the observed behavior "
              "in real market feeds."),
          this, defaults::p90)
    , p99(desc("p99").help(
              "Define the maximum depth of 99% of the events. "
              "The benchmark generates random book changes, with the depth of "
              "these changes controled by this argument (and similar ones), "
              "the default values are chosen to match the observed behavior "
              "in real market feeds."),
          this, defaults::p99)
    , p999(
          desc("p999").help(
              "Define the maximum depth of 99.9% of the events. "
              "The benchmark generates random book changes, with the depth of "
              "these changes controled by this argument (and similar ones), "
              "the default values are chosen to match the observed behavior "
              "in real market feeds."),
          this, defaults::p999)
    , p100(
          desc("p100").help(
              "Define the maximum depth of 100% of the events. "
              "The benchmark generates random book changes, with the depth of "
              "these changes controled by this argument (and similar ones), "
              "the default values are chosen to match the observed behavior "
              "in real market feeds."),
          this, defaults::p100) {
}

void fixture_config::validate() const {

  if (p25() < 0 or p25() > p50()) {
    std::ostringstream os;
    os << "p25 (" << p25() << ") must be >= 0 and > p50 (" << p50() << ")";
    throw jb::usage(os.str(), 1);
  }
  if (p50() < 0 or p50() > p75()) {
    std::ostringstream os;
    os << "p50 (" << p50() << ") must be >= 0 and > p75 (" << p75() << ")";
    throw jb::usage(os.str(), 1);
  }
  if (p75() < 0 or p75() > p90()) {
    std::ostringstream os;
    os << "p75 (" << p75() << ") must be >= 0 and > p90 (" << p90() << ")";
    throw jb::usage(os.str(), 1);
  }
  if (p90() < 0 or p90() > p99()) {
    std::ostringstream os;
    os << "p90 (" << p90() << ") must be >= 0 and > p99 (" << p99() << ")";
    throw jb::usage(os.str(), 1);
  }
  if (p99() < 0 or p99() > p999()) {
    std::ostringstream os;
    os << "p99 (" << p99() << ") must be >= 0 and > p999 (" << p999() << ")";
    throw jb::usage(os.str(), 1);
  }
  if (p999() < 0 or p999() > p100()) {
    std::ostringstream os;
    os << "p999 (" << p999() << ") must be >= 0 and > p100 (" << p100() << ")";
    throw jb::usage(os.str(), 1);
  }
  if (p100() < 0) {
    std::ostringstream os;
    os << "p1000 (" << p100() << ") must be >= 0";
    throw jb::usage(os.str(), 1);
  }
}

config::config()
    : log(desc("log", "logging"), this)
    , microbenchmark(
          desc("microbenchmark", "microbenchmark"), this,
          jb::testing::microbenchmark_config().test_case(defaults::test_case))
    , array_book(desc("array-book"), this)
    , map_book(desc("map-book"), this)
    , fixture(desc("fixture"), this)
    , seed(
          desc("seed").help(
              "Initial seed for pseudo-random number generator. "
              "If zero (the default), use the systems random device to set "
              "the seed."),
          this, 0) {
}

void config::validate() const {
  log().validate();
  array_book().validate();
  map_book().validate();
  fixture().validate();
}
} // anonymous namespace
