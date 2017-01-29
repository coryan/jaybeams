#include <jb/detail/array_traits.hpp>
#include <jb/fftw/aligned_multi_array.hpp>
#include <jb/fftw/tde_result.hpp>

#include <array>
#include <complex>
#include <deque>
#include <forward_list>
#include <list>
#include <vector>

#include <boost/test/unit_test.hpp>

/**
 * @test Verify that we can create a 2 dim tde_result
 */
BOOST_AUTO_TEST_CASE(fftw_tde_result_2_dim_usage) {
  using array_type = jb::fftw::aligned_multi_array<float, 3>;
  using test_tde_result = jb::fftw::aligned_multi_array<std::size_t, 2>;
  using tde_result_type = jb::fftw::tde_result<array_type, std::size_t>;

  // create a tde_result based on array_type
  bool res = std::is_same<tde_result_type::record_type, test_tde_result>::value;
  BOOST_CHECK_MESSAGE(
      res == true, "tde_result<array_type> is not multi array float");

  // create a test timeseries
  const int M = 5;
  const int P = 10;
  const int Q = 20;
  array_type a(boost::extents[M][P][Q]);

  // create a test tde_result
  tde_result_type tde(a);

  // check size of tde = M * P
  BOOST_CHECK_MESSAGE(
      tde.size() == M * P, "tde has an incorrect size=" << tde.size());

  // fill tde result ...
  std::size_t counter = 0;
  for (int i = 0; i != M; ++i) {
    for (int j = 0; j != P; ++j) {
      tde[counter] = counter;
      counter++;
    }
  }

  // ... now check the values
  counter = 0;
  for (int i = 0; i != M; ++i) {
    for (int j = 0; j != P; ++j) {
      BOOST_CHECK_MESSAGE(
          tde[counter] == counter, "tde[" << counter << "] != " << counter);
      counter++;
    }
  }
}

/**
 * @test Verify that we can create a 2 dim tde_result with std:complex values
 */
BOOST_AUTO_TEST_CASE(fftw_tde_result_2_dim_complex_usage) {
  using array_type = jb::fftw::aligned_multi_array<std::complex<float>, 3>;
  using test_tde_result = jb::fftw::aligned_multi_array<std::size_t, 2>;
  using tde_result_type = jb::fftw::tde_result<array_type, std::size_t>;

  // create a tde_result based on array_type
  bool res = std::is_same<tde_result_type::record_type, test_tde_result>::value;
  BOOST_CHECK_MESSAGE(
      res == true, "tde_result<array_type> is not multi array float");

  // create a test timeseries
  const int M = 5;
  const int P = 10;
  const int Q = 20;
  array_type a(boost::extents[M][P][Q]);

  // create a test tde_result
  tde_result_type tde(a);

  // check size of tde = M * P
  BOOST_CHECK_MESSAGE(
      tde.size() == M * P, "tde has an incorrect size=" << tde.size());

  // fill tde result ...
  std::size_t counter = 0;
  for (int i = 0; i != M; ++i) {
    for (int j = 0; j != P; ++j) {
      tde[counter] = counter;
      counter++;
    }
  }

  // ... now check the values
  counter = 0;
  for (int i = 0; i != M; ++i) {
    for (int j = 0; j != P; ++j) {
      BOOST_CHECK_MESSAGE(
          tde[counter] == counter, "tde[" << counter << "] != " << counter);
      counter++;
    }
  }
}

/**
 * @test Verify that we can create a 2 dim tde_result with std:complex values
 * with std::complex result value
 */
BOOST_AUTO_TEST_CASE(fftw_tde_result_2_dim_complex_double_usage) {
  using value_type = std::complex<double>;
  using array_type = jb::fftw::aligned_multi_array<value_type, 3>;
  using test_tde_result = jb::fftw::aligned_multi_array<value_type, 2>;
  using tde_result_type = jb::fftw::tde_result<array_type, value_type>;

  // create a tde_result based on array_type
  bool res = std::is_same<tde_result_type::record_type, test_tde_result>::value;
  BOOST_CHECK_MESSAGE(
      res == true, "tde_result<array_type> is not multi array complex type");

  // create a test timeseries
  const int M = 5;
  const int P = 10;
  const int Q = 20;
  array_type a(boost::extents[M][P][Q]);

  // create a test tde_result
  tde_result_type tde(a);

  // check size of tde = M * P
  BOOST_CHECK_MESSAGE(
      tde.size() == M * P, "tde has an incorrect size=" << tde.size());

  // fill tde result ...
  std::size_t counter = 0;
  for (int i = 0; i != M; ++i) {
    for (int j = 0; j != P; ++j) {
      tde[counter] = value_type(counter, counter);
      counter++;
    }
  }

  // ... now check the values
  counter = 0;
  for (int i = 0; i != M; ++i) {
    for (int j = 0; j != P; ++j) {
      BOOST_CHECK_MESSAGE(
          tde[counter] == value_type(counter, counter),
          "tde[" << counter << "] != " << counter);
      counter++;
    }
  }
}

/**
 * @test Verify that we can create a 1 dim tde_result based on an
 * aligned_multi_array container.
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
  const int M = 50;
  const int P = 100;
  array_type a(boost::extents[M][P]);

  // create a test tde_result
  tde_result_type tde(a);

  // check size of tde = M
  BOOST_CHECK_MESSAGE(
      tde.size() == M, "tde has an incorrect size=" << tde.size());

  // fill tde result ...
  std::size_t counter = 0;
  for (int i = 0; i != M; ++i) {
    tde[counter] = counter;
    counter++;
  }

  // ... now check the values
  counter = 0;
  for (int i = 0; i != M; ++i) {
    BOOST_CHECK_MESSAGE(
        tde[counter] == counter, "tde[" << counter << "] != " << counter);
    counter++;
  }
}

/**
 * @test Verify that we can create a 0 dim tde_result based on a
 * aligned multi array container.
 */
BOOST_AUTO_TEST_CASE(fftw_tde_result_0_dim_multi_array_usage) {
  using array_type = jb::fftw::aligned_multi_array<float, 1>;
  using test_tde_result = std::size_t;
  using tde_result_type = jb::fftw::tde_result<array_type, std::size_t>;

  // create a tde_result based on array_type
  bool res = std::is_same<tde_result_type::record_type, test_tde_result>::value;
  BOOST_CHECK_MESSAGE(res == true, "tde_result<array_type> is not std:size_t");

  // create a test timeseries
  const int M = 1000;
  array_type a(boost::extents[M]);

  // create a test tde_result
  tde_result_type tde(a);

  // check size of tde = q
  BOOST_CHECK_MESSAGE(
      tde.size() == 1, "tde has an incorrect size=" << tde.size());

  // fill tde result ...
  tde[0] = 10;

  // ... now check the values
  BOOST_CHECK_MESSAGE(tde[0] == 10, "tde[0] != 10");
}

/**
 * @test Verify that we can create a 0 dim tde_result based on a array
 * container.
 */
BOOST_AUTO_TEST_CASE(fftw_tde_result_0_dim_array_usage) {
  const int M = 1000;
  using array_type = std::array<float, M>;
  using test_tde_result = std::size_t;
  using tde_result_type = jb::fftw::tde_result<array_type, std::size_t>;

  // create a tde_result based on array_type
  bool res = std::is_same<tde_result_type::record_type, test_tde_result>::value;
  BOOST_CHECK_MESSAGE(res == true, "tde_result<array_type> is not std:size_t");

  // create a test timeseries
  array_type a;

  // create a test tde_result
  tde_result_type tde(a);

  // check size of tde = 1
  BOOST_CHECK_MESSAGE(
      tde.size() == 1, "tde has an incorrect size=" << tde.size());

  // fill tde result ...
  tde[0] = 10;

  // ... now check the values
  BOOST_CHECK_MESSAGE(tde[0] == 10, "tde[0] != 10");
}

/**
 * @test Verify that we can create a 0 dim tde_result based on a vector
 * container.
 */
BOOST_AUTO_TEST_CASE(fftw_tde_result_0_dim_vector_usage) {
  using array_type = std::vector<float>;
  using test_tde_result = std::size_t;
  using tde_result_type = jb::fftw::tde_result<array_type, std::size_t>;

  // create a tde_result based on array_type
  bool res = std::is_same<tde_result_type::record_type, test_tde_result>::value;
  BOOST_CHECK_MESSAGE(res == true, "tde_result<array_type> is not std:size_t");

  // create a test timeseries
  const int M = 1000;
  array_type a(M);

  // create a test tde_result
  tde_result_type tde(a);

  // check size of tde = 1
  BOOST_CHECK_MESSAGE(
      tde.size() == 1, "tde has an incorrect size=" << tde.size());

  // fill tde result ...
  tde[0] = 10;

  // ... now check the values
  BOOST_CHECK_MESSAGE(tde[0] == 10, "tde[0] != 10");
}

/**
 * @test Verify that we can create a 0 dim tde_result based on a deque
 * container.
 */
BOOST_AUTO_TEST_CASE(fftw_tde_result_0_dim_deque_usage) {
  using array_type = std::deque<float>;
  using test_tde_result = std::size_t;
  using tde_result_type = jb::fftw::tde_result<array_type, std::size_t>;

  // create a tde_result based on array_type
  bool res = std::is_same<tde_result_type::record_type, test_tde_result>::value;
  BOOST_CHECK_MESSAGE(res == true, "tde_result<array_type> is not std:size_t");

  // create a test timeseries
  const int M = 1000;
  array_type a(M);

  // create a test tde_result
  tde_result_type tde(a);

  // check size of tde = 1
  BOOST_CHECK_MESSAGE(
      tde.size() == 1, "tde has an incorrect size=" << tde.size());

  // fill tde result ...
  tde[0] = 10;

  // ... now check the values
  BOOST_CHECK_MESSAGE(tde[0] == 10, "tde[0] != 10");
}

/**
 * @test Verify that we can create a 0 dim tde_result based on a
 * forward_list  container.
 */
BOOST_AUTO_TEST_CASE(fftw_tde_result_0_dim_forward_list_usage) {
  using array_type = std::forward_list<float>;
  using test_tde_result = std::size_t;
  using tde_result_type = jb::fftw::tde_result<array_type, std::size_t>;

  // create a tde_result based on array_type
  bool res = std::is_same<tde_result_type::record_type, test_tde_result>::value;
  BOOST_CHECK_MESSAGE(res == true, "tde_result<array_type> is not std:size_t");

  // create a test timeseries
  const int M = 1000;
  array_type a(M);

  // create a test tde_result
  tde_result_type tde(a);

  // check size of tde = 1
  BOOST_CHECK_MESSAGE(
      tde.size() == 1, "tde has an incorrect size=" << tde.size());

  // fill tde result ...
  tde[0] = 10;

  // ... now check the values
  BOOST_CHECK_MESSAGE(tde[0] == 10, "tde[0] != 10");
}

/**
 * @test Verify that we can create a 0 dim tde_result based on a list container.
 */
BOOST_AUTO_TEST_CASE(fftw_tde_result_0_dim_list_usage) {
  using array_type = std::list<float>;
  using test_tde_result = std::size_t;
  using tde_result_type = jb::fftw::tde_result<array_type, std::size_t>;

  // create a tde_result based on array_type
  bool res = std::is_same<tde_result_type::record_type, test_tde_result>::value;
  BOOST_CHECK_MESSAGE(res == true, "tde_result<array_type> is not std:size_t");

  // create a test timeseries
  const int M = 1000;
  array_type a(M);

  // create a test tde_result
  tde_result_type tde(a);

  // check size of tde = 1
  BOOST_CHECK_MESSAGE(
      tde.size() == 1, "tde has an incorrect size=" << tde.size());

  // fill tde result ...
  tde[0] = 10;

  // ... now check the values
  BOOST_CHECK_MESSAGE(tde[0] == 10, "tde[0] != 10");
}
