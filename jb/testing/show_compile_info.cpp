#include <jb/testing/compile_info.hpp>

#include <cstdio>
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) try {
  std::cout << "uname_a: " << jb::testing::uname_a;
  std::cout << "\n\ncompiler: " << jb::testing::compiler;
  std::cout << "\n\ncompiler_flags: " << jb::testing::compiler_flags;
  std::cout << "\n\nlinker: " << jb::testing::linker;
  std::cout << "\n\ngitrev: " << jb::testing::gitrev;
#ifdef __GLIBCPP__
  std::cout << "\n\nlibstdc++ GLIBCPP: " << __GLIBCPP__;
#endif // __GLIBCPP__  
#ifdef __GLIBCXX__
  std::cout << "\n\nlibstdc++ GLIBCXX: " << __GLIBCXX__;
#endif // __GLIBCPP__  
#ifdef _GXX_ABI_VERSION
  std::cout << "\n\nlibstdc++ GXX_ABI_VERSION: " << _GXX_ABI_VERSION;
#endif // _GXX_ABI_VERSION
#ifdef _GLIBCXX_RELEASE
  std::cout << "\n\nlibstdc++ GLIBCXX_RELEASE: " << _GLIBCXX_RELEASE;
#endif // _GLIBCXX_RELEASE
  std::cout << "\n";
  return 0;
} catch (std::exception const& ex) {
  std::cerr << "standard exception raised: " << ex.what() << std::endl;
  return 1;
} catch (...) {
  std::cerr << "unknown exception raised" << std::endl;
  return 1;
}

