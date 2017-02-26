#ifndef jb_testing_microbenchmark_hpp
#define jb_testing_microbenchmark_hpp

#include <jb/detail/reconfigure_thread.hpp>
#include <jb/testing/microbenchmark_base.hpp>

#include <type_traits>
#include <utility>

namespace jb {
namespace testing {

namespace detail {
/**
 * A helper to call Fixture::iteration_setup() in the microbenchmarks.
 *
 * This is the no-op for fixtures that do not provide a
 * iteration_setup() member function.
 *
 * @tparam Fixture the type of the Fixture
 * @tparam Args unused, just a way for SFINAE to fallback on something.
 */
template <typename Fixture, typename... Args>
void call_iteration_setup(Fixture&&, Args&&...) {
}

/**
 * A helper to call Fixture::iteration_setup() in the microbenchmarks.
 *
 * This is the version for fixtures that do provide a
 * iteration_setup() member function.
 *
 * @tparam Fixture the type of the Fixture
 */
template <typename Fixture,
          typename = decltype(std::declval<Fixture>().iteration_setup())>
void call_iteration_setup(Fixture&& fixture) {
  fixture.iteration_setup();
}

/**
 * A helper to call Fixture::iteration_teardown() in the microbenchmarks.
 *
 * This is the no-op for fixtures that do not provide a
 * iteration_teardown() member function.
 *
 * @tparam Fixture the type of the Fixture
 * @tparam Args unused, just a way for SFINAE to fallback on something.
 */
template <typename Fixture, typename... Args>
void call_iteration_teardown(Fixture&&, Args&&...) {
}

/**
 * A helper to call Fixture::iteration_teardown() in the microbenchmarks.
 *
 * This is the version for fixtures that do provide a
 * iteration_teardown() member function.
 *
 * @tparam Fixture the type of the Fixture
 */
template <typename Fixture,
          typename = decltype(std::declval<Fixture>().iteration_teardown())>
void call_iteration_teardown(Fixture&& fixture) {
  fixture.iteration_teardown();
}
} // namespace detail

/**
 * Run a micro-benchmark on a given class.
 *
 * This class is used to run microbenchmarks.  To use it, you wrap
 * your function or class in a Fixture that constructs the unit under
 * test and calls it.  For example, consider a class like this:
 *
 * @code
 * class Foo {
 * public:
 *   Foo(std::string const& need_one_string);
 *
 *   std::string bar(std::string const& need_another_string);
 * };
 * @endcode
 *
 * To test the bar() function you would create a Fixture class like
 * this:
 *
 * @code
 * class FooBarFixture {
 * public:
 *   FooBarFixture(std::string const& pass_on)
 *     : foo_(pass_on), input_string_("some value") {}
 *   void run() const {
 *     foo_.bar(input_string_);
 *   }
 * private:
 *   Foo foo_;
 *   std::string input_string_;
 * };
 * @endcode
 *
 * Then you can run the test using:
 *
 * @code
 * jb::testing::microbenchmark<FooBarFixture> mb(jb::microbenchmark_config());
 *
 * auto results = mb.run(std::string("some other value");
 * jb::microbenchmark_base::summary summary(results);
 * std::cout << summary; // or something more clever...
 * // for a full dump: mb.write_results(std::cout, results);
 * @endcode
 *
 * The framework creates a single instance of the Fixture for the full
 * test, it worries about running a warmup iteration, collecting the
 * results with minimal overhead, and creating a sensible summary.
 *
 * @tparam Fixture A type meeting the following requirements:
 * @code
 * class Fixture {
 * public:
 *   Fixture(Args ...); // default constructor, used for the default sized test
 *   Fixture(int s, Args ...); // constructor for a test of size 's'
 *
 *   int run(); // run one iteration of the test, return the size used
 * };
 * @endcode
 */
template <typename Fixture>
class microbenchmark : public microbenchmark_base {
public:
  typedef jb::testing::microbenchmark_config config;

  /**
   * Default constructor, create a default configuration and
   * initialize from it.
   */
  microbenchmark()
      : microbenchmark(config()) {
  }

  /**
   * Constructor from a configuration
   */
  explicit microbenchmark(config const& cfg)
      : microbenchmark_base(cfg) {
  }

  /**
   * Run the microbenchmaark.
   *
   * @param args the additional arguments, if any, for the Fixture
   * constructor.
   *
   * @tparam Args the types for the additional arguments.
   */
  template <typename... Args>
  results run(Args&&... args) {
    if (config_.reconfigure_thread()) {
      jb::detail::reconfigure_this_thread(config_.thread());
    }
    if (config_.size() != 0) {
      return run_fixed_size(std::forward<Args>(args)...);
    }
    return run_unsized(std::forward<Args>(args)...);
  }

private:
  /**
   * Run a test without specifying the size and some additional
   * arguments for the Fixture constructor.
   *
   * @param args additional arguments for the Fixture
   * @tparam Args the types for the additional arguments
   */
  template <typename... Args>
  results run_unsized(Args&&... args) {
    Fixture fixture(std::forward<Args>(args)...);
    results r;
    run_base(fixture, r);
    return r;
  }

  /**
   * Run a test without specifying the size and no additional
   * arguments for the Fixture constructor.
   */
  results run_unsized() {
    Fixture fixture;
    results r;
    run_base(fixture, r);
    return r;
  }

  /**
   * Run the test when the size is specified in the microbenchmark
   * configuration.
   *
   * @param args additional arguments for the Fixture
   * @tparam Args the types for the additional arguments
   */
  template <typename... Args>
  results run_fixed_size(Args&&... args) {
    results r;
    run_sized(config_.size(), r, std::forward<Args>(args)...);
    return r;
  }

  /**
   * Construct a fixture for the given size and run the  microbenchmkark.
   *
   * @param size the size of the test
   * @param r where to store the results of the test
   * @param args additional arguments for the constructor, if any.
   * @tparam Args the types of the additional arguments, if any.
   */
  template <typename... Args>
  void run_sized(int size, results& r, Args&&... args) {
    Fixture fixture(size, std::forward<Args>(args)...);
    run_base(fixture, r);
  }

  /**
   * Run a microbenchmkark for a constructed fixture.
   *
   * Run the warmup iterations and then run the actual iterations for
   * the test.  The results of the test are ignored during the warmup
   * phase.
   *
   * @param fixture the object under test
   * @param r where to store the results of the test
   */
  void run_base(Fixture& fixture, results& r) {
    for (int i = 0; i != config_.warmup_iterations(); ++i) {
      (void)clock::now();
      detail::call_iteration_setup(fixture);
      (void)fixture.run();
      (void)clock::now();
      detail::call_iteration_teardown(fixture);
    }
    r.reserve(r.size() + config_.iterations());
    for (int i = 0; i != config_.iterations(); ++i) {
      run_iteration(fixture, r);
    }
  }

  /**
   * Run a single iteration of the test and return the results.
   *
   * @param fixture the object under test
   * @param r where to store the results of the test
   */
  static void run_iteration(Fixture& fixture, results& r) {
    detail::call_iteration_setup(fixture);
    auto start = clock::now();
    int size = fixture.run();
    auto stop = clock::now();

    r.emplace_back(std::make_pair(size, stop - start));
    detail::call_iteration_teardown(fixture);
  }
};

} // namespace testing
} // namespace jb

#endif // jb_testing_microbenchmark_hpp
