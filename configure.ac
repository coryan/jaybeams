# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([JayBeams], [0.1], [coryan@users.noreply.github.com])

# Move as many of the generated files as possible to a subdirectory
AC_CONFIG_AUX_DIR(config)
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

# ... the code is going to be C++11, make sure the compiler supports it ...
AX_CXX_COMPILE_STDCXX_11(noext, mandatory)

# ... enable all warnings ...
AX_CXXFLAGS_WARN_ALL

# ... avoid spurious warnings from clang ...
AX_CHECK_COMPILE_FLAG([-Qunused-arguments],
  [AX_APPEND_FLAG(-Qunused-arguments, CXXFLAGS)], [])

# Checks for libraries.
AX_OPENCL([1.2],[],[AC_MSG_ERROR([unable to find a suitable OpenCL library])])

AX_BOOST_BASE([1.55],[],[
  AC_MSG_ERROR([unable to find a suitable Boost library, need version >= 1.55])
])
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

AX_CHECK_LIBRARY(YAML_CPP, [yaml-cpp/yaml.h], [yaml-cpp],
                 [YAML_CPP_LIBS=-lyaml-cpp AC_SUBST(YAML_CPP_LIBS)],
                 [AC_MSG_ERROR([Unable to find yaml-cpp library])])

# FFTW3 Single Precision
AX_CHECK_LIBRARY(FFTW3F, [fftw3.h], [fftw3f],
	[FFTW3F_LIBS=-lfftw3f AC_SUBST(FFTW3F_LIBS)],
        [AC_MSG_ERROR([Unable to find single-precision FFTW3 library])])

# FFTW3 Double Precision
AX_CHECK_LIBRARY(FFTW3, [fftw3.h], [fftw3],
	[FFTW3_LIBS=-lfftw3 AC_SUBST(FFTW3_LIBS)],
        [AC_MSG_ERROR([Unable to find double-precision FFTW3 library])])

# FFTW3 Quad (long double) Precision
AX_CHECK_LIBRARY(FFTW3L, [fftw3.h], [fftw3l],
	[FFTW3L_LIBS=-lfftw3l AC_SUBST(FFTW3L_LIBS)],
        [AC_MSG_ERROR([Unable to find quad-precision FFTW3 library])])

# Checks for header files (and header-only libraries)

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_OUTPUT
