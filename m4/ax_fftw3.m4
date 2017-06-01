#
# SYNOPSIS
#
#   AX_FFTW3([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#
# DESCRIPTION
#
#   Test for the fftw3 libraries.
#
#   If no path to the installed fftw3 library is given the macros seaches
#   under /usr, /usr/local, /opt, and /opt/local.
#
#   This macro calls:
#
#   AC_SUBST(FFTW3_CPPFLAGS) / AC_SUBST(FFTW3_LDFLAGS) / AC_SUBST(FFTW3_LIBS)
#
#   And sets:
#
#   HAVE_FFTW3
#

#serial 1

AC_DEFUN([AX_FFTW3],
[
AC_ARG_WITH([fftw3],
  [AS_HELP_STRING([--with-fftw3@<:@=ARG@:>@],
    [use FFTW3 library from a standard location (ARG=yes),
     from the specified location (ARG=<path>),
     or disable it (ARG=no)
     @<:@ARG=yes@:>@ ])],
    [
    if test "$withval" = "no"; then
        ax_want_fftw3="no"
    elif test "$withval" = "yes"; then
        ax_want_fftw3="yes"
        ax_fftw3_path=""
    else
        ax_want_fftw3="yes"
        ax_fftw3_path="$withval"
    fi
    ],
    [ax_want_fftw3="yes"])


AC_ARG_WITH([fftw3-libdir],
        AS_HELP_STRING([--with-fftw3-libdir=LIB_DIR],
        [Force given directory for fftw3 library. Note that this
         will override library path detection, so use this parameter
         only if default library detection fails and you know exactly
         where your fftw3 library is located.]),
        [
        if test -d "$withval"
        then
                ax_fftw3_lib_path="$withval"
        else
                AC_MSG_ERROR(--with-fftw3-libdir expected directory name)
        fi
        ],
        [ax_fftw3_lib_path=""])


if test "x$ax_want_fftw3" = "xyes"; then
    AC_MSG_CHECKING(for fftw3 libraries)
    succeeded=no

    dnl On 64-bit systems check for both libraries in both the lib and lib64
    dnl directories.
    ax_libsubdirs="lib"
    ax_arch=`uname -m`
    if test $ax_arch = x86_64 -o $ax_arch = ppc64 -o $ax_arch = s390x -o $ax_arch = sparc64; then
        ax_libsubdirs="lib64 lib lib/x86_64"
    fi

    dnl regardless of where the library is, this is the only known name for it
    FFTW3_LIBS="-lfftw3 -lfftw3f -lfftw3l"

    dnl if we were given an specific path, simply use it
    if test "$ax_fftw3_path" != ""; then
      for ax_i in $ax_libsubdirs; do
        if ls "$ax_fftw3_path/$ax_i/libfftw3"* >/dev/null 2>/dev/null ; then break; fi
      done
      FFTW3_LDFLAGS="-L$ax_fftw3_path/$ax_i"
      FFTW3_CPPFLAGS="-I$ax_fftw3_path/include"
    else
      dnl seach the known paths for the header and the libraries
      for ax_path_i in /usr /usr/local /opt /opt/local ; do
        if test -r "$ax_path_i/include/fftw3.h"; then
          for ax_i in $ax_libsubdirs; do
            if ls "$ax_path_i/$ax_i/libfftw3"* >/dev/null 2>/dev/null ; then break; fi
          done
          FFTW3_LDFLAGS="-L$ax_path_i/$ax_i"
          FFTW3_CPPFLAGS="-I$ax_path_i/include"
          break;
        fi
      done
    fi

    dnl overwrite ld flags if the user provided an explicit
    dnl value in the --with-fftw3-libdir parameter
    if test "$ax_fftw3_lib_path" != ""; then
      FFTW3_LDFLAGS="-L$ax_fftw3_lib_path"
    fi

    dnl If the detected CPPFLAGS are a standard path then unset them
    if test "x$FFTW3_CPPFLAGS" = "x-I/usr/include"; then
       FFTW3_CPPFLAGS=""
    fi

    dnl Save the CPPFLAGS and LDFLAGS.  We will modify them during
    dnl testing and eventually we want them to be restored
    CPPFLAGS_SAVED="$CPPFLAGS"
    LDFLAGS_SAVED="$LDFLAGS"

    CPPFLAGS="$CPPFLAGS $FFTW3_CPPFLAGS"
    export CPPFLAGS
    LDFLAGS="$LDFLAGS $FFTW3_LDFLAGS $FFTW3_LIBS"
    export LDFLAGS

    AC_REQUIRE([AC_PROG_CXX])
    AC_LANG_PUSH(C++)

    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        @%:@include <fftw3.h>
        @%:@include <iostream>
        ]], [[
        std::cout << fftw_version << std::endl;
        std::cout << fftwf_version << std::endl;
        std::cout << fftwl_version << std::endl;
        ]]
      )],
      [
        AC_MSG_RESULT(yes)
        succeeded=yes
        found_system=yes
      ],[] )

    AC_LANG_POP([C++])

    if test "$succeeded" != "yes" ; then
      AC_MSG_NOTICE([Cannot detect the fftw3 libraries])
      # execute ACTION-IF-NOT-FOUND (if present):
      ifelse([$2], , :, [$2])
    else
      AC_SUBST(FFTW3_CPPFLAGS)
      AC_SUBST(FFTW3_LDFLAGS)
      AC_SUBST(FFTW3_LIBS)
      AC_DEFINE(HAVE_FFTW3,,[define if the fftw3 libraries are available])
      # execute ACTION-IF-FOUND (if present):
      ifelse([$1], , :, [$1])
    fi

    CPPFLAGS="$CPPFLAGS_SAVED"
    LDFLAGS="$LDFLAGS_SAVED"
fi

])
