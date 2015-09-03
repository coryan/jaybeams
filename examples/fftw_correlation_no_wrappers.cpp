#include <fftw3.h>

#include <chrono>
#include <complex>
#include <iostream>
#include <stdexcept>
#include <vector>

int main(int argc, char* argv[]) try {
  // The size of the test is hardcoded for simplicity's sake ...
  int size = 2048;
  int delay = size / 8;

  std::vector<std::complex<double>> a(size);
  std::vector<std::complex<double>> b(size);
  for (int i = 0; i < size / 4; ++i) {
    a[i] = std::complex<double>(-1, 0);
  }
  for (int i = size / 4; i < 3 * size / 4; ++i) {
    a[i] = std::complex<double>(1, 0);
  }
  for (int i = 3 * size / 4; i < size; ++i) {
    a[i] = std::complex<double>(1, 0);
  }

  for (int i = 0; i < delay; ++i) {
    b[i] = a.at(size - size/8 + i);
  }
  for (int i = delay; i < size; ++i) {
    b[i] = a[i - delay];
  }

  std::vector<std::complex<double>> ta(a.size());
  std::vector<std::complex<double>> tb(a.size());
  fftw_plan ffta = fftw_plan_dft_1d(
      a.size(), reinterpret_cast<fftw_complex*>(&a[0]),
      reinterpret_cast<fftw_complex*>(&ta[0]), FFTW_FORWARD, FFTW_ESTIMATE);
  fftw_plan fftb = fftw_plan_dft_1d(
      b.size(), reinterpret_cast<fftw_complex*>(&b[0]),
      reinterpret_cast<fftw_complex*>(&tb[0]), FFTW_FORWARD, FFTW_ESTIMATE);
  std::vector<std::complex<double>> prod(size);
  std::vector<std::complex<double>> correlation(a.size());
  fftw_plan inv = fftw_plan_dft_1d(
      a.size(), reinterpret_cast<fftw_complex*>(&prod[0]),
      reinterpret_cast<fftw_complex*>(&correlation[0]), FFTW_BACKWARD,
      FFTW_ESTIMATE);

  auto start = std::chrono::steady_clock::now();
  fftw_execute(ffta);
  fftw_execute(fftb);

  for (std::size_t i = 0; i != ta.size(); ++i) {
    prod[i] = std::conj(ta[i]) * tb[i];
  }


  fftw_execute(inv);

  std::size_t argmax = 0;
  double max = std::numeric_limits<double>::min();
  for (std::size_t i = 0; i != correlation.size(); ++i) {
    if (max < correlation[i].real()) {
      max = correlation[i].real();
      argmax = i;
    }
  }
  auto end = std::chrono::steady_clock::now();
  fftw_destroy_plan(inv);
  fftw_destroy_plan(fftb);
  fftw_destroy_plan(ffta);

  std::cout << "delay=" << delay << ", argmax=" << argmax
            << ", max=" << max << std::endl
            << "elapsed="
            << std::chrono::duration_cast<std::chrono::microseconds>(
                end - start).count()
            << std::endl;
  

  return 0;
} catch(std::exception const& ex) {
  std::cerr << "standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch(...) {
  std::cerr << "unknown exception raised" << std::endl;
  return 1;
}
