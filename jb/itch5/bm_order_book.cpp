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
#include <jb/testing/microbenchmark.hpp>
#include <jb/histogram.hpp>
#include <jb/integer_range_binning.hpp>
#include <jb/log.hpp>

#include <algorithm>
#include <functional>
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

  jb::config_attribute<fixture_config, int> max_p999_delta;

  jb::config_attribute<fixture_config, int> min_qty;
  jb::config_attribute<fixture_config, int> max_qty;
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

/// A simple representation for book operations.
struct operation {
  /// The price to modify
  jb::itch5::price4_t px;
  /// The quantity, negative values indicate the operation is a
  /// remove_order(), positive that it is an add_order() operation.
  int delta;
};

/**
 * Initialize a PRNG based on the seed command-line argument.
 *
 * If seed is 0 the PRNG is initialized using std::random_device.
 * That provides a lot of entropy into the PRNG.  But sometimes we
 * want to run the same test over and over, with predictable results,
 * for example to debug the benchmark, or to fine-tune the operating
 * system parameters with a consistent benchmark.  In that case we can
 * provide a seed to initialize the generator.
 *
 * One should not use the generator initialized with a 32-bit seed for
 * anything that requires good statistical properties in the generated
 * numbers.
 *
 * @param seed the seed parameter as documented above.
 * @return a generator initialized with @a seed if seed is not zero,
 *   and initialized with the output from std::random_device if it is
 *   zero.
 */
std::mt19937_64 initialize_generator(int seed);

/**
 * Create a sequence of operations.
 *
 * @param seed a seed for the PRNG, or 0 if the generator should be
 *   initialized from std::random_device.
 * @param size the number of elements in the sequence
 * @param cfg describe the statistical properties of the sequence
 * @param is_ascending if true generate operations for a BUY book.
 */
std::vector<operation> create_operations(
    std::mt19937_64& generator, int size, fixture_config const& cfg,
    bool is_ascending);

template <typename order_book_side, typename book_config>
std::function<void()> create_iteration(
    int seed, int size, fixture_config const& cfg, book_config const& bkcfg) {
  auto generator = initialize_generator(seed);
  order_book_side bk(bkcfg);
  std::vector<operation> ops =
      create_operations(generator, size, cfg, bk.is_ascending());

  auto lambda =
      [ book = std::move(bk), operations = std::move(ops) ]() mutable {
    // ... iterate over the operations and pass them to the book ...
    for (auto const& op : operations) {
      if (op.delta < 0) {
        book.reduce_order(op.px, -op.delta);
      } else {
        book.add_order(op.px, op.delta);
      }
    }
  };

  return std::function<void()>(std::move(lambda));
}

/// The default number of iterations for the benchmark
static int constexpr default_size = 20000;

/// Run the benchmark for a specific book type
template <typename order_book, typename book_config>
class fixture {
public:
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
      : size_(size)
      , cfg_(cfg)
      , bkcfg_(bkcfg)
      , seed_(seed)
      , generator_(initialize_generator(seed_)) {
  }

  void iteration_setup() {
    std::uniform_int_distribution<> side(0, 1);
    if (side(generator_)) {
      iteration_ = create_iteration<typename order_book::buys_t>(
          seed_, size_, cfg_, bkcfg_);
    } else {
      iteration_ = create_iteration<typename order_book::sells_t>(
          seed_, size_, cfg_, bkcfg_);
    }
  }

  /// Run an iteration of the test ...
  void run() {
    iteration_();
  }

private:
  /// The size for the test
  int size_;

  /// The configuration for this fixture
  fixture_config cfg_;

  /// The configuration for the book side
  book_config bkcfg_;

  /// The seed to initialize the generator for each iteration
  int seed_;

  /// The PRNG, initialized based on the seed parameter
  std::mt19937_64 generator_;

  /// Store the state and functions to execute in the next iteration
  std::function<void()> iteration_;
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
template <typename book_type, typename book_type_config>
void run_benchmark(config const& cfg, book_type_config const& book_cfg) {
  JB_LOG(info) << "Running benchmark for " << cfg.microbenchmark().test_case()
               << " with SEED=" << cfg.seed();
  using benchmark =
      jb::testing::microbenchmark<fixture<book_type, book_type_config>>;
  benchmark bm(cfg.microbenchmark());
  auto r = bm.run(cfg.fixture(), book_cfg, cfg.seed());

  typename benchmark::summary s(r);
  // ... print the summary and full results to std::cout, without
  // using JB_LOG() because this is parsed by the driver script ...
  std::cerr << cfg.microbenchmark().test_case() << " summary " << s
            << std::endl;
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
    JB_LOG(info) << "Configuration for test\n" << cfg << "\n";
    JB_LOG(info) << "  default_size=" << default_size;
  }

  using namespace jb::itch5;
  auto test_case = cfg.microbenchmark().test_case();
  if (test_case == "array") {
    run_benchmark<array_based_order_book>(cfg, cfg.array_book());
  } else if (test_case == "map") {
    run_benchmark<map_based_order_book>(cfg, cfg.map_book());
  } else {
    // ... it is tempting to move this code to the validate() member
    // function, but then we have to repeat the valid configurations
    // twice in the code ...
    std::ostringstream os;
    os << "Unknown test case (" << test_case << ")" << std::endl;
    os << " --microbenchmark.test-case must be one of"
       << ": array, map" << std::endl;
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
#ifndef JB_ITCH5_DEFAULT_bm_order_book_test_case
#define JB_ITCH5_DEFAULT_bm_order_book_test_case "array"
#endif // JB_ITCH5_DEFAULT_bm_order_book_test_case

#ifndef JB_ITCH5_DEFAULT_bm_order_book_p25
#define JB_ITCH5_DEFAULT_bm_order_book_p25 0
#endif // JB_ITCH5_DEFAULT_bm_order_book_p25

#ifndef JB_ITCH5_DEFAULT_bm_order_book_p50
#define JB_ITCH5_DEFAULT_bm_order_book_p50 1
#endif // JB_ITCH5_DEFAULT_bm_order_book_p50

#ifndef JB_ITCH5_DEFAULT_bm_order_book_p75
#define JB_ITCH5_DEFAULT_bm_order_book_p75 6
#endif // JB_ITCH5_DEFAULT_bm_order_book_p75

#ifndef JB_ITCH5_DEFAULT_bm_order_book_p90
#define JB_ITCH5_DEFAULT_bm_order_book_p90 14
#endif // JB_ITCH5_DEFAULT_bm_order_book_p90

#ifndef JB_ITCH5_DEFAULT_bm_order_book_p99
#define JB_ITCH5_DEFAULT_bm_order_book_p99 203
#endif // JB_ITCH5_DEFAULT_bm_order_book_p99

#ifndef JB_ITCH5_DEFAULT_bm_order_book_p999
#define JB_ITCH5_DEFAULT_bm_order_book_p999 2135
#endif // JB_ITCH5_DEFAULT_bm_order_book_p999

#ifndef JB_ITCH5_DEFAULT_bm_order_book_p100
#define JB_ITCH5_DEFAULT_bm_order_book_p100 20000000
#endif // JB_ITCH5_DEFAULT_bm_order_book_p100

#ifndef JB_ITCH5_DEFAULT_bm_order_book_max_p999_delta
#define JB_ITCH5_DEFAULT_bm_order_book_max_p999_delta 200
#endif // JB_ITCH5_DEFAULT_bm_order_book_max_p999_delta

#ifndef JB_ITCH5_DEFAULT_bm_order_book_min_qty
#define JB_ITCH5_DEFAULT_bm_order_book_min_qty -5000
#endif // JB_ITCH5_DEFAULT_bm_order_book_min_qty

#ifndef JB_ITCH5_DEFAULT_bm_order_book_max_qty
#define JB_ITCH5_DEFAULT_bm_order_book_max_qty 5000
#endif // JB_ITCH5_DEFAULT_bm_order_book_max_qty

std::string const test_case = JB_ITCH5_DEFAULT_bm_order_book_test_case;
int constexpr p25 = JB_ITCH5_DEFAULT_bm_order_book_p25;
int constexpr p50 = JB_ITCH5_DEFAULT_bm_order_book_p50;
int constexpr p75 = JB_ITCH5_DEFAULT_bm_order_book_p75;
int constexpr p90 = JB_ITCH5_DEFAULT_bm_order_book_p90;
int constexpr p99 = JB_ITCH5_DEFAULT_bm_order_book_p99;
int constexpr p999 = JB_ITCH5_DEFAULT_bm_order_book_p999;
int constexpr p100 = JB_ITCH5_DEFAULT_bm_order_book_p100;

int constexpr max_p999_delta = JB_ITCH5_DEFAULT_bm_order_book_max_p999_delta;

int constexpr min_qty = JB_ITCH5_DEFAULT_bm_order_book_min_qty;
int constexpr max_qty = JB_ITCH5_DEFAULT_bm_order_book_max_qty;
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
          this, defaults::p100)
    , max_p999_delta(
          desc("max-p999-delta")
              .help(
                  "The maximum delta for the desired vs. actual event depth "
                  "distribution at the p99.9 level. "
                  "The benchmark generates random book changes, with the "
                  "distribution of the event depths controlled by the "
                  "--pXYZ arguments. "
                  "Any generated set of book changes whose p99.9 differs by "
                  "more than this value from the requested value is rejected, "
                  "and a new set of book changes is generated."),
          this, defaults::max_p999_delta)
    , min_qty(
          desc("min-qty").help(
              "Generate book changes uniformly distributed between "
              "--min-qty and --max-qty (both inclusive."),
          this, defaults::min_qty)
    , max_qty(
          desc("max-qty").help(
              "Generate book changes uniformly distributed between "
              "--min-qty and --max-qty (both inclusive."),
          this, defaults::max_qty) {
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

  if (max_p999_delta() < 0 or max_p999_delta() > p999() / 2) {
    std::ostringstream os;
    os << "max-p999-delta (" << max_p999_delta() << ") must be > 0"
       << " and must be <= p999/2 (" << p999() / 2 << ")";
    throw jb::usage(os.str(), 1);
  }

  if (min_qty() >= 0) {
    std::ostringstream os;
    os << "min-qty (" << min_qty() << ") must be < 0";
    throw jb::usage(os.str(), 1);
  }

  if (max_qty() <= 0) {
    std::ostringstream os;
    os << "max-qty (" << max_qty() << ") must be > 0";
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

std::mt19937_64 initialize_generator(int seed) {
  // ... the operations are generated based on a PRNG, we use one of
  // the standard generators from the C++ library ...
  std::mt19937_64 generator;

  if (seed != 0) {
    // ... the user wants a repeatable (but not well randomized) test,
    // this is useful when comparing micro-optimizations to see if
    // anything has changed, or when measuring the performance of the
    // microbenchmark framework itself ...
    //
    // BTW, I am aware (see [1]) of the deficiencies in initializing a
    // Mersenne-Twister with only 32 bits of seed, in this case I am
    // not looking for a statistically unbiased, nor a
    // cryptographically strong PRNG.  We just want something that can
    // be easily controlled from the command line....
    //
    // [1]: http://www.pcg-random.org/posts/cpp-seeding-surprises.html
    //
    generator.seed(seed);
  } else {
    // ... in this case we make every effort to initialize from a good
    // source of entropy.  There are no guarantees that
    // std::random_device is a good source of entropy, but for the
    // platforms JayBeams uses it is ...
    std::random_device rd;
    // ... the mt19937 will call the SeedSeq generate() function N*K
    // times, where:
    //   N = generator.state_size
    //   K = ceil(generator.word_size / 32)
    // details in:
    //   http://en.cppreference.com/w/cpp/numeric/random/mersenne_twister_engine
    // so we populate the SeedSeq with that many words, tedious as
    // that is.  The SeedSeq will do complicated math to mix the input
    // and ensure all the bits are distributed across as much of the
    // space as possible, even with a single word of input.  But it is
    // better to start that seed_seq with more entropy ...
    //
    // [2]: http://www.pcg-random.org/posts/cpps-random_device.html
    //
    auto S = generator.state_size * (generator.word_size / 32);
    std::vector<unsigned int> seeds(S);
    std::generate(seeds.begin(), seeds.end(), [&rd]() { return rd(); });
    // ... std::seed_seq is an implementation of the SeedSeq concept,
    // which cleverly remixes its input in case we got numbers in a
    // small range ...
    std::seed_seq seq(seeds.begin(), seeds.end());
    // ... and then use the std::seed_seq to fill the generator ...
    generator.seed(seq);
  }
  return generator;
}

std::vector<operation> create_operations_without_validation(
    std::mt19937_64& generator, int& actual_p999, int size,
    fixture_config const& cfg, bool is_ascending) {

  // ... we will save the operations here, and return them at the end ...
  std::vector<operation> operations;
  operations.reserve(size);

  // ... use the configuration parameters to create a "depth
  // distribution".  The default values are chosen from
  // realistic market data, so the benchmark should generate data
  // that is statistically similar to what the market shows ...
  std::vector<int> boundaries(
      {0, cfg.p25(), cfg.p50(), cfg.p75(), cfg.p90(), cfg.p99(), cfg.p999(),
       cfg.p100()});
  std::vector<double> weights({0.25, 0.25, 0.25, 0.15, 0.09, 0.009, 0.001});
  JB_ASSERT_THROW(boundaries.size() == weights.size() + 1);
  std::piecewise_constant_distribution<> ddis(
      boundaries.begin(), boundaries.end(), weights.begin());

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
  if (is_ascending) {
    level2price = [](int level) {
      return jb::itch5::level_to_price<price4_t>(level);
    };
  } else {
    level2price = [max_level](int level) {
      return jb::itch5::level_to_price<price4_t>(max_level - level);
    };
  }

  // ... we keep a histogram of the intended depths so we can verify
  // that we are generating a valid simulation ...
  jb::histogram<jb::integer_range_binning<int>> book_depth_histogram(
      jb::integer_range_binning<int>(0, 8192));
  jb::histogram<jb::integer_range_binning<int>> qty_histogram(
      jb::integer_range_binning<int>(cfg.min_qty(), cfg.max_qty()));

  // ... keep track of the contents in a simulated book, indexed by
  // price level ...
  std::map<int, int> book;

  for (int i = 0; i != size; ++i) {
    if (book.empty()) {
      // ... generate the initial order randomly, as long as it has a
      // legal price level and a qty in the configured range ...
      auto const initial_qty =
          std::uniform_int_distribution<>(1, cfg.max_qty())(generator);
      auto const initial_level =
          std::uniform_int_distribution<>(1, max_level - 1)(generator);
      operations.push_back({level2price(initial_level), initial_qty});
      book[initial_level] = initial_qty;
      book_depth_histogram.sample(0);
      qty_histogram.sample(initial_qty);
      continue;
    }
    // ... find out what is the current best level, or use
    // initial_level if it is not available ...
    int best_level = book.rbegin()->first;
    // ... generate a new level to operate at ...
    int depth = static_cast<int>(ddis(generator));
    int level;
    // ... pick if the operation will be above or below the current
    // level ...
    if (std::uniform_int_distribution<>(0, 1)(generator)) {
      level = best_level - depth;
    } else {
      level = best_level + depth;
    }
    // ... normalize the level to the valid range ...
    if (level <= 0) {
      level = 1;
    } else if (level >= max_level) {
      level = max_level - 1;
    }

    // ... generate the qty, but make sure it is valid, first
    // establish the minimum value we can generate ...
    int min_qty;
    auto f = book.find(level);
    if (f != book.end()) {
      min_qty = std::max(cfg.min_qty(), -f->second);
    } else {
      // ... empty level, the operation must add something ...
      min_qty = 1;
    }
    // ... then keep generating values until we are in range, oh,
    // and zero is not a valid value ...
    int qty = 0;
    while (qty == 0) {
      // ... for the size, we use a uniform distribution in the
      // [cfg.min_qty(), cfg.max_qty()] range, though we have to trim
      // the results based on what is actually in the book ...
      qty = std::uniform_int_distribution<>(min_qty, cfg.max_qty())(generator);
    }
    // ... now insert the operation into the array, and record it in
    // the book ...
    book[level] += qty;
    book_depth_histogram.sample(depth);
    qty_histogram.sample(qty);
    if (book[level] == 0) {
      book.erase(level);
    }
    operations.push_back({level2price(level), qty});
  }

  JB_LOG(trace) << "Simulated depth histogram: "
                << book_depth_histogram.summary();
  JB_LOG(trace) << "Desired: " << jb::histogram_summary{0.0,
                                                        double(cfg.p25()),
                                                        double(cfg.p50()),
                                                        double(cfg.p75()),
                                                        double(cfg.p90()),
                                                        double(cfg.p99()),
                                                        double(cfg.p100()),
                                                        1};
  JB_LOG(trace) << "Simulated qty histogram: " << qty_histogram.summary();

  actual_p999 = book_depth_histogram.estimated_quantile(0.999);
  return operations;
}

std::vector<operation> create_operations(
    std::mt19937_64& generator, int size, fixture_config const& cfg,
    bool is_ascending) {

  int const min_p999 = cfg.p999() - cfg.max_p999_delta();
  int const max_p999 = cfg.p999() + cfg.max_p999_delta();
  int actual_p999 = 0;
  std::vector<operation> operations;
  int count = 0;
  do {
    if (actual_p999 != 0) {
      JB_LOG(trace) << "retrying for p999 = " << actual_p999
                    << ", count=" << ++count;
    }
    operations = create_operations_without_validation(
        generator, actual_p999, size, cfg, is_ascending);
  } while (actual_p999 < min_p999 or max_p999 < actual_p999);

  return operations;
}

} // anonymous namespace
