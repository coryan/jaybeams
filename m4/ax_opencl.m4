#
# SYNOPSIS
#
#   AX_OPENCL([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#
# DESCRIPTION
#
#   Test for the OpenCL C++ library.
#
#   If no path to the installed OpenCL library is given the macros seaches
#   under /usr/local, /opt, /opt/local.
#
#   This macro calls:
#
#   AC_SUBS(OPENCL_CPPFLAGS) / AC_SUBS(OPENCL_LDFLAGS)
#
#   And sets:
#
#   HAVE_OPENCL
#   HAVE_OPENCL_APPLE_HEADER
#

#serial 1

AC_DEFUN([AX_OPENCL],
[
AC_ARG_WITH([opencl],
  [AS_HELP_STRING([--with-opencl@<:@=ARG@:>@],
    [use OpenCL library from a standard location (ARG=yes),
     from the specified location (ARG=<path>),
     or disable it (ARG=no)
     @<:@ARG=yes@:>@ ])],
    [
    if test "$withval" = "no"; then
        ax_want_opencl="no"
    elif test "$withval" = "yes"; then
        ax_want_opencl="yes"
        ax_opencl_path=""
    else
        ax_want_opencl="yes"
        ax_opencl_path="$withval"
    fi
    ],
    [ax_want_opencl="yes"])


AC_ARG_WITH([opencl-libdir],
        AS_HELP_STRING([--with-opencl-libdir=LIB_DIR],
        [Force given directory for OpenCL library. Note that this
         will override library path detection, so use this parameter
         only if default library detection fails and you know exactly
         where your OpenCL library is located.]),
        [
        if test -d "$withval"
        then
                ax_opencl_lib_path="$withval"
        else
                AC_MSG_ERROR(--with-opencl-libdir expected directory name)
        fi
        ],
        [ax_opencl_lib_path=""])


if test "x$ax_want_opencl" = "xyes"; then
    opencl_version_req=ifelse([$1], ,1.0,$1)
    WANT_OPENCL_VERSION=`echo CL_VERSION_${opencl_version_req} | sed -e 's/\./_/g'  `
    AC_MSG_CHECKING(for OpenCL and OpenCL library >= $opencl_version_req )
    succeeded=no

    dnl On 64-bit systems check for both libraries in both the lib and lib64
    dnl directories.
    ax_libsubdirs="lib lib/fglrx"
    ax_arch=`uname -m`
    if test $ax_arch = x86_64 -o $ax_arch = ppc64 -o $ax_arch = s390x -o $ax_arch = sparc64; then
        ax_libsubdirs="lib64 lib lib/x86_64 lib/fglrx"
    fi

    dnl regardless of where the library is, this is the only known name for it
    OPENCL_LIB="-lOpenCL"

    dnl if we were given an specific path, simply use it
    if test "$ax_opencl_path" != ""; then
      for ax_i in $ax_libsubdirs; do
        if ls "$ax_opencl_path/$ax_i/libOpenCL"* >/dev/null 2>/dev/null ; then break; fi
      done
      OPENCL_LDFLAGS="-L$ax_opencl_path/$ax_i"
      OPENCL_CPPFLAGS="-I$ax_opencl_path/include"
    else
      dnl seach the known paths for the header and the libraries
      for ax_path_i in /usr /usr/local /opt /opt/local ; do
        if test -r "$ax_path_i/include/CL/cl.hpp" || test -r "$ax_path_i/include/OpenCL/cl.hpp" ; then
          for ax_i in $ax_libsubdirs; do
            if ls "$ax_path_i/$ax_i/libOpenCL"* >/dev/null 2>/dev/null ; then break; fi
          done
          OPENCL_LDFLAGS="-L$ax_path_i/$ax_i"
          OPENCL_CPPFLAGS="-I$ax_path_i/include"
          break;
        fi
      done
    fi

    dnl overwrite ld flags if the user provided an explicit
    dnl value in the --with-opencl-libdir parameter
    if test "$ax_opencl_lib_path" != ""; then
      OPENCL_LDFLAGS="-L$ax_opencl_lib_path"
    fi

    dnl Save the CPPFLAGS and LDFLAGS.  We will modify them during
    dnl testing and eventually we want them to be restored
    CPPFLAGS_SAVED="$CPPFLAGS"
    LDFLAGS_SAVED="$LDFLAGS"

    CPPFLAGS="$CPPFLAGS $OPENCL_CPPFLAGS"
    export CPPFLAGS
    LDFLAGS="$LDFLAGS $OPENCL_LDFLAGS $OPENCL_LIB"
    export LDFLAGS

    AC_REQUIRE([AC_PROG_CXX])
    AC_LANG_PUSH(C++)

    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        @%:@include <CL/cl.hpp>
        @%:@include <vector>
        ]], [[
        #if defined($WANT_OPENCL_VERSION) && $WANT_OPENCL_VERSION == 1
        // Everything is okay
        #else
        #  error OpenCL version is too old
        #endif
        cl::Context context(CL_DEVICE_TYPE_CPU, 0, nullptr, nullptr);
        (void) context.getInfo<CL_CONTEXT_DEVICES>();
        ]]
      )],
      [
        AC_MSG_RESULT(yes)
        succeeded=yes
        found_system=yes
        found_apple_header=no
      ],[] )

    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        @%:@include <OpenCL/cl.hpp>
        @%:@include <vector>
        ]], [[
        #if defined($WANT_OPENCL_VERSION) && $WANT_OPENCL_VERSION == 1
        // Everything is okay
        #else
        #  error OpenCL version is too old
        #endif
        cl::Context context(CL_DEVICE_TYPE_CPU, 0, nullptr, nullptr);
        (void) context.getInfo<CL_CONTEXT_DEVICES>();
        ]]
      )],
      [
        AC_MSG_RESULT(yes)
        succeeded=yes
        found_system=yes
        found_apple_header=yes
      ],[] )
    AC_LANG_POP([C++])

    if test "$succeeded" != "yes" ; then
      AC_MSG_NOTICE([Cannot detect the OpenCL library])
      # execute ACTION-IF-NOT-FOUND (if present):
      ifelse([$3], , :, [$3])
    else
      AC_SUBST(OPENCL_CPPFLAGS)
      AC_SUBST(OPENCL_LDFLAGS)
      AC_SUBST(OPENCL_LIB)
      AC_DEFINE(HAVE_OPENCL,,[define if the OpenCL library is available])
      if test "$found_apple_header" == "yes"; then
        AC_DEFINE(HAVE_OPENCL_APPLE_HEADER,,
                  [define if cl.hpp is found under OpenCL/])
      fi
      # execute ACTION-IF-FOUND (if present):
      ifelse([$2], , :, [$2])
    fi

    CPPFLAGS="$CPPFLAGS_SAVED"
    LDFLAGS="$LDFLAGS_SAVED"
fi

])
