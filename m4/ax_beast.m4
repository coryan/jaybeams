#
# SYNOPSIS
#
#   AX_BEAST([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#
# DESCRIPTION
#
#   Test for the Beast HTTP and WebSocket library
#   (https://github.com/vinniefalco/Beast).  Beast is part of Boost
#   incubator, but it is not clear when, or if, it will be included in
#   Boost.
#
#   This function calls:
#         AC_SUBST(BEAST_CPPFLAGS)
#
#   and sets:
#       HAVE_BEAST
#

#serial 1

AC_DEFUN([AX_BEAST],
[
AC_ARG_WITH([beast],
  [AS_HELP_STRING([--with-beast@<:@=ARG@:>@],
    [use Beast HTTP library from a standard location (ARG=yes),
     from the specified location (ARG=<path>), or disable it (ARG=no)
     @<:@ARG=yes@:>@ ])
  ],
  [if test "$withval" = "no"; then
     ax_want_beast="no"
   elif test "$withval" = "yes"; then
     ax_want_beast="yes"
     ax_beast_path=""
    else
      ax_want_beast="yes"
      ax_beast_path="$withval"
   fi
  ],
  [ax_want_beast="yes"]
)

if test "x$ax_want_beast" = "xyes"; then
  AC_MSG_CHECKING([for Beast HTTP library])
  succeeded=no

  dnl if we were given an specific path, simply use it
  ax_possible_include_subdirs="include"
  if test "$ax_beast_path" != ""; then
    for ax_i in $ax_possible_include_subdirs; do
      if test -r "$ax_beast_path/$ax_i/beast/core.hpp"; then break; fi
    done
    BEAST_CPPFLAGS="-I$ax_beast_path/$ax_i"
  else
    dnl seach the known paths for the Boost.Compute header
    ax_initial_searchdirs="/usr /usr/local /opt /opt/local"
    ax_search_dirs=$ax_initial_searchdirs
    for ax_path_i in $ax_initial_searchdirs; do
      found=`ls -dr1 $ax_path_i/boost-compute* 2>/dev/null`
      ax_search_dirs="$ax_search_dirs $found"
    done
    for ax_path_i in $ax_search_dirs ; do
      for ax_i in $ax_possible_include_subdirs; do
        if test -r "$ax_path_i/$ax_i/beast/core.hpp"; then break; fi
      done
      BEAST_CPPFLAGS="-I$ax_path_i/$ax_i"
      if test -r "$ax_path_i/$ax_i/beast/core.hpp"; then break; fi
    done
  fi

  dnl If the detected CPPFLAGS are a standard path then unset them
  if test "x$BEAST_CPPFLAGS" = "x-I/usr/include"; then
     BEAST_CPPFLAGS=""
  fi

  dnl Save the CPPFLAGS and LDFLAGS.  We will modify them during
  dnl testing and eventually we want them to be restored
  CPPFLAGS_SAVED="$CPPFLAGS"
  LDFLAGS_SAVE="$LDFLAGS"

  CPPFLAGS="$CPPFLAGS $OPENCL_CPPFLAGS $BEAST_CPPFLAGS"
  export CPPFLAGS
  LDFLAGS="$LDFLAGS $OPENCL_LDFLAGS $OPENCL_LIB"
  export LDFLAGS

  AC_REQUIRE([AC_PROG_CXX])
  AC_LANG_PUSH(C++)
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
      @%:@include <beast/core.hpp>
      @%:@include <beast/http.hpp>
      @%:@include <iostream>
      ]],
      [[
        beast::http::request<beast::http::string_body> req;
        std::cout << sizeof(req) << std::endl;
      ]]
    )],
    [
      AC_MSG_RESULT(yes)
      succeeded=yes
      found_system=yes
    ],[] )
  AC_LANG_POP([C++])

  if test "$succeeded" != "yes" ; then
    AC_MSG_NOTICE([Cannot detect the Beast HTTP library])
    # execute ACTION-IF-NOT-FOUND (if present):
    ifelse([$2], , :, [$2])
  else
    AC_SUBST(BEAST_CPPFLAGS)
    AC_DEFINE(HAVE_BEAST,,
       [define if the Beast HTTP library is available])
    # execute ACTION-IF-FOUND (if present):
    ifelse([$1], , :, [$1])
  fi

  CPPFLAGS="$CPPFLAGS_SAVED"
  LDFLAGS="$LDFLAGS_SAVED"
fi

])
