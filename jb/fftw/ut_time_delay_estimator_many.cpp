#include <jb/fftw/aligned_multi_array.hpp>
#include <jb/fftw/plan.hpp>
#include <jb/testing/create_square_timeseries.hpp>
#include <jb/testing/create_triangle_timeseries.hpp>
#include <jb/testing/delay_timeseries.hpp>

#include <boost/test/unit_test.hpp>
#include <chrono>

/**
 * Helpers to test jb::time_delay_estimator_many
 */
namespace {} // anonymous namespace

namespace jb {
namespace fftw {

template <typename array_t, template <typename T, std::size_t K>
                            class multi_array = jb::fftw::aligned_multi_array>
class time_delay_estimator_many {
public:
};

} // namespace fftw
} // namespace jb

/**
 * @test Verify that we can create and use a simple time delay estimator.
 */
BOOST_AUTO_TEST_CASE(fftw_time_delay_estimator_many_simple) {
  int const nsamples = 1 << 15;
  int const delay = 1250;
  int const S = 128;
  using timeseries_type = jb::fftw::aligned_multi_array<float, 2>;
  using tested = jb::fftw::time_delay_estimator_many<timeseries_type>;

  timeseries_type a(boost::extents[S][nsamples]);
  timeseries_type b(boost::extents[S][nsamples]);
  int count = 0;
  for (auto vector : a) {
    if (++count % 2 == 0) {
      jb::testing::create_triangle_timeseries(nsamples, vector);
    } else {
      jb::testing::create_square_timeseries(nsamples, vector);
    }
  }
  for (int i = 0; i != S; ++i) {
    for (int j = 0; j != nsamples; ++j) {
      b[i][j] = a[i][(j + delay) % nsamples];
    }
  }
}
