# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([JayBeams], [0.1], [coryan@users.noreply.github.com])

# Move as many of the generated files as possible to a subdirectory
AC_CONFIG_AUX_DIR(config)

# ... include the m4 subdirectory in the search path for macros ...
AC_CONFIG_MACRO_DIR([m4])

# Yes, this is a C++ project.
AC_LANG([C++])

# We are going to use Automake to generate the makefiles, so initialize it
AM_INIT_AUTOMAKE([subdir-objects foreign])

# make Automake silent
AM_SILENT_RULES([yes])

# Create a config header, but with a distinctive name
AM_CONFIG_HEADER([jb/jb-config.hpp])

# There is only one Makefile in this project, it is non-recursive.
AC_CONFIG_FILES([Makefile])

# Make sure there is a C++ compiler, it supports C++11 and enable all warnings.
AC_PROG_CXX
# ... make sure we know how to create static libraries ...
AC_PROG_RANLIB

# ... on Fedora the default ARFLAGS/AR_FLAGS produce warnings ...
ARFLAGS=cr
AR_FLAGS=cr
AC_SUBST(ARFLAGS)
AC_SUBST(AR_FLAGS)

# ... check if there is a clang-format installed, allow the user to
# override the default by setting the CLANG_FORMAT environment
# variable ...
AC_CHECK_PROGS(CLANG_FORMAT, $CLANG_FORMAT clang-format, "")
AC_MSG_CHECKING([checking for clang-format version >= 3.8.0])
AS_IF([test "x$CLANG_FORMAT" != "x"], [
  ac_clang_format_version=`$CLANG_FORMAT --version | $AWK '{print $3}'`
  AX_COMPARE_VERSION([$ac_clang_format_version], [ge], [3.8.0],
    [AM_CONDITIONAL([FOUND_CLANG_FORMAT], [true])
     AC_MSG_RESULT([yes])],
    [AM_CONDITIONAL([FOUND_CLANG_FORMAT], [false])
     AC_MSG_RESULT([no])]
  )
], [
  AM_CONDITIONAL([FOUND_CLANG_FORMAT], [false])
  AC_MSG_RESULT([no])
])

# ... the code is going to be C++11, make sure the compiler supports it ...
AX_CXX_COMPILE_STDCXX_14(noext, mandatory)

# ... enable all warnings ...
AX_CXXFLAGS_WARN_ALL

# ... avoid spurious warnings from clang ...
AX_CHECK_COMPILE_FLAG([-Qunused-arguments],
  [AX_APPEND_FLAG(-Qunused-arguments, CXXFLAGS)], [])

# ... enable static linking if the user requests it ...
AX_CONFIG_FEATURE_DEFAULT_DISABLED
AX_CONFIG_FEATURE(static-linking, [turns on/off static linking],
                  HAVE_STATIC_LINKING,
                  [Define if you want all programs to be statically linked],
                  [enable_static_linking="yes"], [enable_static_linking="no"])
AM_CONDITIONAL(STATIC_LINKING, [test "$enable_static_linking" = "yes"])

# ... we need Boost with a version higher than 1.55 because Boost.Log ...
AX_BOOST_BASE([1.55], [], [
    AC_MSG_ERROR([
        unable to find a suitable Boost library, need version >= 1.55.
    ])
])

# ... then check for each specific Boost.* library we use ...
AX_BOOST_ASIO
AX_BOOST_LOG
AX_BOOST_LOG_SETUP
AX_BOOST_FILESYSTEM
AX_BOOST_SYSTEM
AX_BOOST_DATE_TIME
AX_BOOST_THREAD
AX_BOOST_IOSTREAMS
AX_BOOST_PROGRAM_OPTIONS
AX_BOOST_UNIT_TEST_FRAMEWORK

AX_PTHREAD([], [AC_MSG_ERROR([unable to find the Pthreads library.]) ])

# ... find out if OpenCL is installed, enable it by default, but users can
# disable OpenCL (and anything that depends on it) using --without-opencl ...
AX_OPENCL([1.1], [], [
    AC_MSG_ERROR([unable to find a suitable OpenCL library])
])

# ... create a conditional for automake ..
AM_CONDITIONAL([HAVE_OPENCL], [test "x$ax_want_opencl" = "xyes"])

# ... we want to use clFFT and Boost.Compute only if OpenCL is enabled ...
AM_COND_IF([HAVE_OPENCL], [
    AX_CLFFT([], [
        AC_MSG_ERROR([Unable to find clFFT library.])
    ])
    AX_BOOST_COMPUTE([], [
        AC_MSG_ERROR([Unable to find Boost.Compute library.])
    ])
    # ... Boost.Compute uses deprecated features of OpenCL 1.1, so we
    # have to disable warnings about them if we end up using both ...
    AX_CHECK_COMPILE_FLAG([-Wno-deprecated-declarations],
       [AX_APPEND_FLAG(-Wno-deprecated-declarations, CXXFLAGS)], [])
    AM_COND_IF([STATIC_LINKING], [
      OPENCL_LIB="-Wl,--push-state,-Bdynamic,-lOpenCL,--pop-state"
      CLFFT_LIB="-Wl,--push-state,-Bdynamic,-lclFFT,--pop-state"
      AC_SUBST(OPENCL_LIB)
      AC_SUBST(CLFFT_LIB)
    ], [])
])


# ... YAML-CPP is a library to parse YAML files ...
AX_CHECK_LIBRARY(YAML_CPP, [yaml-cpp/yaml.h], [yaml-cpp],
    [YAML_CPP_LIBS=-lyaml-cpp AC_SUBST(YAML_CPP_LIBS)],
    [AC_MSG_ERROR([Unable to find yaml-cpp library])
])

# ... require FFTW3 by default, but allow the user to disable it ...
AX_FFTW3([], [])
AM_CONDITIONAL([USE_FFTW3], [test "x$ax_want_fftw3" = "xyes"])

# ... libbz2 is a dependency for Boost.IOStreams when compiled statically ...
AX_CHECK_LIBRARY(BZIP2, [bzlib.h], [bz2],
  [BZIP2_LIBS=-lbz2 AC_SUBST(BZIP2_LIBS)],
  [AC_MSG_ERROR([Unable to find bzip2 library])
])

# ... libz is a dependency for Boost.IOStreams when compiled statically ...
AX_CHECK_LIBRARY(LIBZ, [zlib.h], [z],
  [LIBZ_LIBS=-lz AC_SUBST(LIBZ_LIBS)],
  [AC_MSG_ERROR([Unable to find libz library])
])

# ... turn all warnings into errors.  We want clean builds because
# warnings are a good way to detect portability problems.  Set this at
# the end, because some of the library detection builds fail otherwise
# ...
AX_CHECK_COMPILE_FLAG([-Werror], [AX_APPEND_FLAG(-Werror, CXXFLAGS)] [])

AC_OUTPUT
