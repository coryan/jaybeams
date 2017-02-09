#ifndef jb_testing_compile_info_hpp
#define jb_testing_compile_info_hpp

namespace jb {
namespace testing {

/// The kernel version, release, machine hardware, etc.
extern char const uname_a[];

/// The compiler version
extern char const compiler[];

/// The compiler flags
extern char const compiler_flags[];

/// The linker version
extern char const linker[];

/// The git revision at compile time
extern char const gitrev[];

} // namespace testing
} // namespace jb

#endif // jb_testing_compile_info_hpp
