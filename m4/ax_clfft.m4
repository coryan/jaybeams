#
# SYNOPSIS
#
#   AX_CLFFT([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#
# DESCRIPTION
#
#   Test for the clFFT C++ library.
#
#   If no path to the installed clFFT library is given the macros seaches
#   under /usr, /usr/local, /opt, /opt/local, /usr/clFFT*,
#   /usr/local/clFFT*,  and /opt/local/clFFT*.  If there are multiple
#   matches the one with the highest (alphabetic) version number is
#   chosen.
#
#   This macro calls:
#
#   AC_SUBS(CLFFT_CPPFLAGS) / AC_SUBS(CLFFT_LDFLAGS)
#
#   And sets:
#
#   HAVE_CLFFT
#

#serial 1

AC_DEFUN([AX_CLFFT],
[
AC_ARG_WITH([clfft],
  [AS_HELP_STRING([--with-clfft@<:@=ARG@:>@],
    [use clFFT library from a standard location (ARG=yes),
     from the specified location (ARG=<path>),
     or disable it (ARG=no)
     @<:@ARG=yes@:>@ ])],
    [
    if test "$withval" = "no"; then
        ax_want_clfft="no"
    elif test "$withval" = "yes"; then
        ax_want_clfft="yes"
        ax_clfft_path=""
    else
        ax_want_clfft="yes"
        ax_clfft_path="$withval"
    fi
    ],
    [ax_want_clfft="yes"])


AC_ARG_WITH([clfft-libdir],
        AS_HELP_STRING([--with-clfft-libdir=LIB_DIR],
        [Force given directory for clFFT library. Note that this
         will override library path detection, so use this parameter
         only if default library detection fails and you know exactly
         where your clFFT library is located.]),
        [
        if test -d "$withval"
        then
                ax_clfft_lib_path="$withval"
        else
                AC_MSG_ERROR(--with-clfft-libdir expected directory name)
        fi
        ],
        [ax_clfft_lib_path=""])


if test "x$ax_want_clfft" = "xyes"; then
    AC_MSG_CHECKING(for clFFT library)
    succeeded=no

    dnl On 64-bit systems check for both libraries in both the lib and lib64
    dnl directories.
    ax_libsubdirs="lib"
    ax_arch=`uname -m`
    if test $ax_arch = x86_64 -o $ax_arch = ppc64 -o $ax_arch = s390x -o $ax_arch = sparc64; then
        ax_libsubdirs="lib64 lib"
    fi

    dnl regardless of where the library is, this is the only known name for it
    CLFFT_LIB="-lclFFT"

    dnl if we were given an specific path, simply use it
    if test "$ax_clfft_path" != ""; then
      for ax_i in $ax_libsubdirs; do
        if ls "$ax_clfft_path/$ax_i/libclFFT"* >/dev/null 2>/dev/null ; then break; fi
      done
      CLFFT_LDFLAGS="-L$ax_clfft_path/$ax_i"
      CLFFT_CPPFLAGS="-I$ax_clfft_path/include"
    else
      dnl seach the known paths for the header and the libraries
      ax_initial_searchdirs="/usr /usr/local /opt /opt/local"
      ax_search_dirs=""
      for ax_path_i in $ax_initial_searchdirs; do
        found=`ls -dr1 $ax_path_i/clFFT* 2>/dev/null`
        ax_search_dirs="$ax_search_dirs $found"
      done
      for ax_path_i in $ax_search_dirs ; do
        if test -r "$ax_path_i/include/clFFT.h" ; then
          for ax_i in $ax_libsubdirs; do
            if ls "$ax_path_i/$ax_i/libclFFT"* >/dev/null 2>/dev/null ; then break; fi
          done
          CLFFT_LDFLAGS="-L$ax_path_i/$ax_i"
          CLFFT_CPPFLAGS="-I$ax_path_i/include"
          break;
        fi
      done
    fi

    dnl overwrite ld flags if the user provided an explicit
    dnl value in the --with-clfft-libdir parameter
    if test "$ax_clfft_lib_path" != ""; then
      CLFFT_LDFLAGS="-L$ax_clfft_lib_path"
    fi

    dnl Save the CPPFLAGS and LDFLAGS.  We will modify them during
    dnl testing and eventually we want them to be restored
    CPPFLAGS_SAVED="$CPPFLAGS"
    LDFLAGS_SAVED="$LDFLAGS"

    CPPFLAGS="$CPPFLAGS $OPENCL_CPPFLAGS $CLFFT_CPPFLAGS"
    export CPPFLAGS
    LDFLAGS="$LDFLAGS $OPENCL_LDLAGS $CLFFT_LDFLAGS $CLFFT_LIB $OPENCL_LIB"
    export LDFLAGS

    AC_REQUIRE([AC_PROG_CXX])
    AC_LANG_PUSH(C++)
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        @%:@include <clFFT.h>
        ]], [[
        #if defined(clfftVersionMajor) && defined(clfftVersionMinor)
          cl_int err;

          cl_platform_id platform = 0;
          err = clGetPlatformIDs(1, &platform, NULL);

          clfftSetupData fft_setup;
          err = clfftInitSetupData(&fft_setup);
        #else
        #  error clFFT version not defined
        #endif
        ]]
      )],
      [
        AC_MSG_RESULT(yes)
        succeeded=yes
        found_system=yes
      ],[] )
    AC_LANG_POP([C++])

    if test "$succeeded" != "yes" ; then
      AC_MSG_NOTICE([Cannot detect the clFFT library])
      # execute ACTION-IF-NOT-FOUND (if present):
      ifelse([$2], , :, [$2])
    else
      AC_SUBST(CLFFT_CPPFLAGS)
      AC_SUBST(CLFFT_LDFLAGS)
      AC_SUBST(CLFFT_LIB)
      AC_DEFINE(HAVE_CLFFT,,[define if the clFFT library is available])
      # execute ACTION-IF-FOUND (if present):
      ifelse([$1], , :, [$1])
    fi

    CPPFLAGS="$CPPFLAGS_SAVED"
    LDFLAGS="$LDFLAGS_SAVED"
fi

])
