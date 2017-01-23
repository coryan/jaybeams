#include <jb/detail/array_traits.hpp>
#include <jb/fftw/aligned_multi_array.hpp>
#include <jb/fftw/tde_result.hpp>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that we can create a 2 dim tde_result
 */
BOOST_AUTO_TEST_CASE(fftw_tde_result_2_dim_usage) {
  using array_type = jb::fftw::aligned_multi_array<float, 3>;
  using test_tde_result = jb::fftw::aligned_multi_array<std::size_t, 2>;
  using tde_result_type = jb::fftw::tde_result<array_type, std::size_t>;
  using tde_result_type = jb::fftw::tde_result<array_type, std::size_t>;

  // create a tde_result based on array_type
  bool res = std::is_same<tde_result_type::record_type, test_tde_result>::value;
  BOOST_CHECK_MESSAGE(
      res == true, "tde_result<array_type> is not multi array float");

  // create a test timeseries
  const int O = 10;
  const int P = 10;
  const int Q = 10;
  array_type a(boost::extents[O][P][Q]);

  // create a test tde_result
  tde_result_type tde(a);

  // fill tde result ...
  std::size_t counter = 0;
  for (int i = 0; i != O; ++i) {
    for (int j = 0; j != P; ++j) {
      tde[counter] = counter;
      counter++;
    }
  }

  // ... now check the values
  counter = 0;
  for (int i = 0; i != O; ++i) {
    for (int j = 0; j != P; ++j) {
      BOOST_CHECK_MESSAGE(
          tde[counter] == counter, "tde[" << counter << "] != " << counter);
      counter++;
    }
  }
}

/**
 * @test Verify that we can create a 1 dim tde_result
 */
BOOST_AUTO_TEST_CASE(fftw_tde_result_1_dim_usage) {
  using array_type = jb::fftw::aligned_multi_array<float, 2>;
  using test_tde_result = jb::fftw::aligned_multi_array<std::size_t, 1>;
  using tde_result_type = jb::fftw::tde_result<array_type, std::size_t>;

  // create a tde_result based on array_type
  bool res = std::is_same<tde_result_type::record_type, test_tde_result>::value;
  BOOST_CHECK_MESSAGE(
      res == true, "tde_result<array_type> is not multi array float");

  // create a test timeseries
  const int O = 10;
  const int P = 10;
  array_type a(boost::extents[O][P]);

  // create a test tde_result
  tde_result_type tde(a);

  // fill tde result ...
  std::size_t counter = 0;
  for (int i = 0; i != O; ++i) {
    tde[counter] = counter;
    counter++;
  }

  // ... now check the values
  counter = 0;
  for (int i = 0; i != O; ++i) {
    BOOST_CHECK_MESSAGE(
        tde[counter] == counter, "tde[" << counter << "] != " << counter);
    counter++;
  }
}

/**
 * @test Verify that we can create a 0 dim tde_result
 */
BOOST_AUTO_TEST_CASE(fftw_tde_result_0_dim_usage) {
  using array_type = jb::fftw::aligned_multi_array<float, 1>;
  using test_tde_result = std::size_t;
  using tde_result_type = jb::fftw::tde_result<array_type, std::size_t>;

  // create a tde_result based on array_type
  bool res = std::is_same<tde_result_type::record_type, test_tde_result>::value;
  BOOST_CHECK_MESSAGE(res == true, "tde_result<array_type> is not std:size_t");

  // create a test timeseries
  const int O = 10;
  array_type a(boost::extents[O]);

  // create a test tde_result
  tde_result_type tde(a);

  // fill tde result ...
  tde[0] = 10;

  // ... now check the values
  BOOST_CHECK_MESSAGE(tde[0] == 10, "tde[0] != 10");
}
