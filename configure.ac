# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# Copyright 2015-2017 Carlos O'Ryan
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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

# ... TODO(#157) the generated Makefile uses GNU Make features, for
# complicated reasons.  Make sure we can find it ...
AX_CHECK_GNU_MAKE
AS_IF([test "x$ifGNUmake" == "x"], [], [
  AC_MSG_ERROR([Cannot find GNU Make, aborting.])])

# ... on Fedora the default ARFLAGS/AR_FLAGS produce warnings ...
ARFLAGS=cr
AR_FLAGS=cr
AC_SUBST(ARFLAGS)
AC_SUBST(AR_FLAGS)

# ... enable, if possible, valgrind checking ...
AX_VALGRIND_CHECK

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
  [AX_APPEND_FLAG(-Qunused-arguments, AM_CXXFLAGS)], [])

# ... enable static linking if the user requests it ...
AX_CONFIG_FEATURE_DEFAULT_DISABLED
AX_CONFIG_FEATURE(static-linking, [turns on/off static linking],
                  HAVE_STATIC_LINKING,
                  [Define if you want all programs to be statically linked],
                  [enable_static_linking="yes"], [enable_static_linking="no"])
AM_CONDITIONAL(STATIC_LINKING, [test "$enable_static_linking" = "yes"])

# ... enable the AddressSantizer if the user requests it (and the
# compiler supports it) ...
AC_ARG_WITH([sanitizer],
    [AS_HELP_STRING([--with-sanitizer=(address|undefined|memory|...)],
        [Enable sanitized builds.  Some tests are disabled in these configs.])],
    [SANITIZER="$withval"], [])

AS_IF([test "x$SANITIZER" != "x"], [
    AX_CHECK_COMPILE_FLAG([-fsanitize=$SANITIZER],
        [AX_APPEND_FLAG(-fsanitize=$SANITIZER, AM_CXXFLAGS)
         HAVE_SANITIZER=$SANITIZER
         AC_SUBST(HAVE_SANITIZER)
         AM_CONDITIONAL([HAVE_SANITIZER], [true])],
        [AC_MSG_ERROR([Compiler does not support the $SANITIZER sanitizer.])]
    )],
    [AM_CONDITIONAL([HAVE_SANITIZER], [false])]
)


AS_IF([test "x$SANITIZER" == "xaddress"],
    [HAVE_SANITIZER_ADDRESS=yes
     AC_DEFINE(HAVE_SANTIZER_ADDRESS,,
        [define if built with address sanitizer])
     AC_SUBST(HAVE_SANITIZER_ADDRESS)], [])
AS_IF([test "x$SANITIZER" == "xmemory"],
    [HAVE_SANITIZER_MEMORY=yes
     AC_DEFINE(HAVE_SANTIZER_MEMORY,,
        [define if built with memory sanitizer])
     AC_SUBST(HAVE_SANITIZER_MEMORY)], [])
AS_IF([test "x$SANITIZER" == "xundefined"],
    [HAVE_SANITIZER_UNDEFINED=yes
     AC_DEFINE(HAVE_SANTIZER_UNDEFINE,,
        [define if built with undefined behavior sanitizer])
     AC_SUBST(HAVE_SANITIZER_UNDEFINED)], [])
AS_IF([test "x$SANITIZER" == "xthread"],
    [HAVE_SANITIZER_THREAD=yes
     AC_DEFINE(HAVE_SANTIZER_THREAD,,
        [define if built with thread sanitizer])
     AC_SUBST(HAVE_SANITIZER_THREAD)], [])

# ... find out where the Protobuf and gRPC libraries can be found
# using pkg-confg.  Avoid using PKG_CHECK_MODULES because we want
# separate -I, -L, and -l flags ...
PKG_PROG_PKG_CONFIG
if $PKG_CONFIG --atleast-version=3.3 protobuf; then
  PROTOBUF_LIBS=`$PKG_CONFIG --libs protobuf`
  PROTOBUF_CPPFLAGS=`$PKG_CONFIG --cflags-only-I protobuf`
  PROTOBUF_CFLAGS=`$PKG_CONFIG --cflags-only-other protobuf`
else
  AC_MSG_ERROR([Cannot find protobuf >= 3.3])
fi
AC_SUBST(PROTOBUF_LIBS)
AC_SUBST(PROTOBUF_CPPFLAGS)
AC_SUBST(PROTOBUF_CFLAGS)

if $PKG_CONFIG --atleast-version=1.3.4 grpc++; then
  GRPCPP_LIBS=`$PKG_CONFIG --libs grpc++ grpc`
  GRPCPP_CPPFLAGS=`$PKG_CONFIG --cflags-only-I grpc++ grpc`
  GRPCPP_CFLAGS=`$PKG_CONFIG --cflags-only-other grpc++ grpc`
else
  AC_MSG_ERROR([Cannot find grpc++ >= 1.3.4])
fi
AC_SUBST(GRPCPP_LIBS)
AC_SUBST(GRPCPP_CPPFLAGS)
AC_SUBST(GRPCPP_CFLAGS)

AC_ARG_WITH([protoc],
    [AS_HELP_STRING([--with-protoc=/protoc/full/path],
        [Set the location of the protocol buffers compiler (protoc).
         Defaults to looking on the PATH.])],
    [PROTOC="$withval"],
    [ AS_IF([test "x$PROTOC" == "x"],
        [AC_PATH_PROG([PROTOC], [protoc], [no])])
    ]
)
AS_IF([test "$PROTOC" == "no"],
    [AC_MSG_ERROR([ProtoBuf compiler "protoc" not found.])])

AC_ARG_WITH([grpc_cpp_plugin],
    [AS_HELP_STRING([--with-grpc-cpp-plugin=/grpc_cpp_plugin/full/path],
        [Set the location of the gRPC C++ plugin for protoc.
         Defaults to looking on the PATH.])],
    [GRPC_CPP_PLUGIN="$withval"],
    [ AS_IF([test "x$GRPC_CPP_PLUGIN" == "x"],
        [AC_PATH_PROG([GRPC_CPP_PLUGIN], [grpc_cpp_plugin], [no])])
    ]
)
AS_IF([test "$GRPC_CPP_PLUGIN" == "no"],
    [AC_MSG_ERROR([gRPC C++ plugin not found.])])

# ... we need Boost with a version higher than 1.58 because Beast and
# Boost.Endian ...
AX_BOOST_BASE([1.58], [], [
    AC_MSG_ERROR([
        unable to find a suitable Boost library, need version >= 1.58.
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
    AM_COND_IF([STATIC_LINKING], [
      OPENCL_LIB="-Wl,--push-state,-Bdynamic,-lOpenCL,--pop-state"
      CLFFT_LIB="-Wl,--push-state,-Bdynamic,-lclFFT,--pop-state"
      AC_SUBST(OPENCL_LIB)
      AC_SUBST(CLFFT_LIB)
    ], [])
])

# ... Beast (https://github.com/vinniefalco/Beast) for http ...
AX_BEAST([], [
        AC_MSG_ERROR([Unable to find Beast HTTP library.])
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
# TODO() - use AC_LANG_WERROR() instead ?
AX_CHECK_COMPILE_FLAG([-Werror], [AX_APPEND_FLAG(-Werror, AM_CXXFLAGS)] [])

# ... monkey patch the -Wno-* flags depending on what warnings are
# triggered by Boost.Compute ...
AC_LANG_PUSH(C++)
CPPFLAGS_SAVED="$CPPFLAGS"
LDFLAGS_SAVE="$LDFLAGS"

CPPFLAGS="$CPPFLAGS $OPENCL_CPPFLAGS $BOOST_COMPUTE_CPPFLAGS $AM_CXXFLAGS"
export CPPFLAGS
LDFLAGS="$LDFLAGS $OPENCL_LDFLAGS $OPENCL_LIB"
export LDFLAGS

AC_MSG_CHECKING([for deprecated-declarations warning in Boost.Compute])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
      @%:@include <boost/compute.hpp>
      @%:@include <vector>
    ]], [[
      namespace bc = boost::compute;
      std::vector<bc::platform> platforms = bc::system::platforms();
    ]]
  )], [
    # ... success, nothing to do ...
    AC_MSG_RESULT(none)
  ], [
    # ... failure, most likely the -Wno-deprecated-declarations flag.
    # Boost.Compute triggers this flag because it uses OpenCL 1.1
    # constructs that are deprecated ...
    AC_MSG_RESULT(yes)
    AX_CHECK_COMPILE_FLAG([-Wno-deprecated-declarations],
       [AX_APPEND_FLAG(-Wno-deprecated-declarations, AM_CXXFLAGS)], [])
  ] )

CPPFLAGS="$CPPFLAGS_SAVED $OPENCL_CPPFLAGS $BOOST_COMPUTE_CPPFLAGS $AM_CXXFLAGS"
AC_MSG_CHECKING([for ignored-attributes warning in Boost.Compute])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
      @%:@include <boost/compute.hpp>
      @%:@include <vector>
    ]], [[
      namespace bc = boost::compute;
      std::vector<bc::ulong_> things(16);
      (void)things.size();
    ]]
  )], [
    # ... success, nothing to do ...
    AC_MSG_RESULT(none)
  ], [
    # ... failure, most likely compiling with Mesa, which sets
    # alignment attributes in the aliases for cl_ulong and similar
    # types, but those aliases are ignored when used as template
    # parameters ...
    AC_MSG_RESULT(yes)
    AX_CHECK_COMPILE_FLAG([-Wno-ignored-attributes],
       [AX_APPEND_FLAG(-Wno-ignored-attributes, AM_CXXFLAGS)], [])
  ] )

# ... check the final result ...
AC_MSG_CHECKING([for clean build with Boost.Compute])
CPPFLAGS="$CPPFLAGS_SAVED $OPENCL_CPPFLAGS $BOOST_COMPUTE_CPPFLAGS $AM_CXXFLAGS"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
      @%:@include <boost/compute.hpp>
      @%:@include <vector>
    ]], [[
      namespace bc = boost::compute;
      std::vector<bc::platform> platforms = bc::system::platforms();
      std::vector<boost::compute::ulong_> things(16);
      (void)things.size();
    ]]
  )], [
    # ... success, nothing to do ...
    AC_MSG_RESULT(yes)
  ], [
    # ... we need to change the configuration ...
    AC_MSG_ERROR([Cannot compile Boost.Compute with all warnings enabled])
  ] )
CPPFLAGS="$CPPFLAGS_SAVED"
LDFLAGS="$LDFLAGS_SAVED"
AC_LANG_POP([C++])

AC_SUBST(AM_CXXFLAGS)
AC_SUBST(AM_CPPFLAGS)

AC_OUTPUT
