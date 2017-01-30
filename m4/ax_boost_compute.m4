#
# SYNOPSIS
#
#   AX_BOOST_COMPUTE([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
#
# DESCRIPTION
#
#   Test for the Boost.Compute library.  Boost.Compute "A C++ GPU
#   Computing Library for OpenCL".  Boost.Compute is not yet part of
#   Boost, so it must be compiled, installed, and located separately
#   from the rest of Boost.
#
#   This function calls:
#         AC_SUBST(BOOST_COMPUTE_CPPFLAGS)
#
#   and sets:
#       HAVE_BOOST_COMPUTE
#

#serial 1

AC_DEFUN([AX_BOOST_COMPUTE],
[
AC_ARG_WITH([boost-compute],
  [AS_HELP_STRING([--with-boost-compute@<:@=ARG@:>@],
    [use Boost.Compute library from a standard location (ARG=yes),
     from the specified location (ARG=<path>), or disable it (ARG=no)
     @<:@ARG=yes@:>@ ])
  ],
  [if test "$withval" = "no"; then
     ax_want_boost_compute="no"
   elif test "$withval" = "yes"; then
     ax_want_boost_compute="yes"
     ax_boost_compute_path=""
    else
      ax_want_boost_compute="yes"
      ax_boost_compute_path="$withval"
   fi
  ],
  [ax_want_boost_compute="yes"]
)

if test "x$ax_want_boost_compute" = "xyes"; then
  AC_MSG_CHECKING([for Boost.Compute library])
  succeeded=no

  dnl if we were given an specific path, simply use it
  dnl ... or not that simply: Boost.Compute installs itself in
  dnl     $prefix/include/compute/boost/compute/compute.hpp
  dnl that makes usage very weird compared to other libraries, unless
  dnl you add -I$prefix/include/compute
  ax_possible_include_subdirs="include/compute include"
  if test "$ax_boost_compute_path" != ""; then
    for ax_i in $ax_possible_include_subdirs; do
      if test -r "$ax_boost_compute_path/$ax_i/boost/compute.hpp"; then break; fi
    done
    BOOST_COMPUTE_CPPFLAGS="-I$ax_boost_compute_path/$ax_i"
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
        if test -r "$ax_path_i/$ax_i/boost/compute.hpp"; then break; fi
      done
      BOOST_COMPUTE_CPPFLAGS="-I$ax_path_i/$ax_i"
      if test -r "$ax_path_i/$ax_i/boost/compute.hpp"; then break; fi
    done
  fi

  dnl Save the CPPFLAGS and LDFLAGS.  We will modify them during
  dnl testing and eventually we want them to be restored
  CPPFLAGS_SAVED="$CPPFLAGS"
  LDFLAGS_SAVE="$LDFLAGS"

  CPPFLAGS="$CPPFLAGS $OPENCL_CPPFLAGS $BOOST_COMPUTE_CPPFLAGS"
  export CPPFLAGS
  LDFLAGS="$LDFLAGS $OPENCL_LDLAGS $OPENCL_LIB"
  export LDFLAGS

  AC_REQUIRE([AC_PROG_CXX])
  AC_LANG_PUSH(C++)
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
      @%:@include <boost/compute.hpp>
      ]],
      [[
        namespace bc = boost::compute;
        std::vector<bc::platform> platforms = bc::system::platforms();
      ]]
    )],
    [
      AC_MSG_RESULT(yes)
      succeeded=yes
      found_system=yes
    ],[] )
  AC_LANG_POP([C++])

  if test "$succeeded" != "yes" ; then
    AC_MSG_NOTICE([Cannot detect the Boost.Compute library])
    # execute ACTION-IF-NOT-FOUND (if present):
    ifelse([$2], , :, [$2])
  else
    AC_SUBST(BOOST_COMPUTE_CPPFLAGS)
    AC_DEFINE(HAVE_BOOST_COMPUTE,,
       [define if the Boost.Compute library is available])
    # execute ACTION-IF-FOUND (if present):
    ifelse([$1], , :, [$1])
  fi

  CPPFLAGS="$CPPFLAGS_SAVED"
  LDFLAGS="$LDFLAGS_SAVED"
fi

])
