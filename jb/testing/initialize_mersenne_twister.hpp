#ifndef jb_testing_initialize_mersenne_twister_hpp
#define jb_testing_initialize_mersenne_twister_hpp

#include <algorithm>
#include <random>
#include <string>

namespace jb {
namespace testing {

char const default_initialization_marker[] = "__default__";

/**
 * Initialize a Mersenne-Twister based generator.
 *
 * Often tests need to initialize a PRNG based on either a
 * command-line argument (to make tests reproduceable), or from the
 * std::random_device (to have good distribution properties in the
 * PRGN).  This is a fairly tedious initialization, so we have
 * refactored it to a common function.
 *
 * @param seed the command-line argument seed, 0 means initialize from
 * std::random_device.
 * @param token the parameter to initialize std::random_device, if
 * __default__ then the random device is default initialized.  The
 * semantics of @a token are platform specific, but on Linux it is
 * usually the device to read random numbers from (/dev/urandom or
 * /dev/random).  It is very rare to need /dev/random, specially in
 * tests, where this function is most commonly used.
 * @return a Generator initialized as described above.
 * @throws std::exception if the token is invalid on the platform.
 *
 * @tparam Generator an instantiation of
 * std::mersenne_twister_generator<>, most likely std::mt19937 or
 * std::mt19937_64.
 */
template <typename Generator>
Generator initialize_mersenne_twister(int seed, std::string const& token) {
  // ... the operations are generated based on a PRNG, we use one of
  // the standard generators from the C++ library ...
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
    return Generator(seed);
  }

  // ... in this case we make every effort to initialize from a good
  // source of entropy ...

  // First, we need to know how much entropy will be needed by the
  // Generator.
  // A Mersenne-Twister generator will call the SeedSeq generate()
  // function N*K times, where:
  //   N = generator.state_size
  //   K = ceil(generator.word_size / 32)
  // details in:
  //   http://en.cppreference.com/w/cpp/numeric/random/mersenne_twister_engine
  // so we populate the SeedSeq with that many words, tedious as
  // that is.
  //
  // [2]: http://www.pcg-random.org/posts/cpps-random_device.html

  // ... compute how many words we need ...
  auto const S = Generator::state_size * (Generator::word_size / 32);
  // ... create a vector to hold the entropy, or what we believe is
  // entropy ...
  std::vector<unsigned int> entropy(S);
  if (token == default_initialization_marker) {
    // ... notice that the C++ standard does not guarantee that
    // std::random_device produces actual random numbers, but with the
    // compilers and libraries that JayBeams is compiled against there
    // is good reason [3] to believe they are ...
    //
    // [3]:
    // http://en.cppreference.com/w/cpp/numeric/random/random_device/random_device
    //
    std::random_device rd;
    std::generate(entropy.begin(), entropy.end(), [&rd]() { return rd(); });
  } else {
    std::random_device rd(token);
    std::generate(entropy.begin(), entropy.end(), [&rd]() { return rd(); });
  }

  // ... std::seed_seq is an implementation of the SeedSeq concept,
  // which cleverly remixes its input in case we got numbers in a
  // small range.  It will do complicated math to mix the input
  // and ensure all the bits are distributed across as much of the
  // space as possible, even with a single word of input ...
  std::seed_seq seq(entropy.begin(), entropy.end());
  return Generator(seq);
}

} // namespace testing
} // namespace jb

#endif // jb_testing_initialize_mersenne_twister_hpp
