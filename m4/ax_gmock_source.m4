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
#
# SYNOPSIS
#
#   AX_CHECK_GMOCK_SOURCE(
#      [FILENAMES], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND], [DIRS])
#
# DESCRIPTION
#
#   Find the gmock headers and source code.  The Google Mocking
#   library (gmock, http://github.com/googlemock/googlemock) is often
#   distributed in source form because the application must compile it
#   with the right options.  This macro finds the source code and
#   verifies it can be compiled.
#
#   If successful, AC_CHECK_GMOCK_SOURCE sets AX_GMOCK_SOURCE to the
#   name of the file found, and AX_GMOCK_SOURCE_CPPFLAGS to the
#   pre-processor flags used to successfully compile it.  No attempt
#   is made to *link* the file.
#
#

#serial 1

AC_DEFUN([AX_CHECK_GMOCK_SOURCE], [
  AC_REQUIRE([AC_PROG_CXX])
  AC_LANG_PUSH(C++)
  got=no
  for fil in $1; do
   src=`echo $fil | $as_tr_sh`
   for dir in - $4; do
    if test "x${got}" = "xno"; then
      ax_hassrc_cvdir=`echo $dir | $as_tr_sh`
      AC_CACHE_CHECK([for $fil source in $dir],
        [ax_cv${ax_hassrc_cvdir}_hassrc_${src}],
        [ ax_have_src_save_cflags=${CXXFLAGS}
          gmock_cxxflags=""
          if test "$dir" != "-"; then
            gmock_cxxflags="-I$dir"
          fi
          CXXFLAGS="$CXXFLAGS $gmock_cxxflags"
          AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
@%:@include <$fil>
              ]]
            )],
            [got="yes"; eval "ax_cv${ax_hassrc_cvdir}_hassrc_${src}"="yes"],
            [got="no"; eval "ax_cv${ax_hassrc_cvdir}_hassrc_${src}"="no"])
          CXXFLAGS=$ax_have_src_save_cflags
      ])
      if eval `echo 'test x${'ax_cv${ax_hassrc_cvdir}_hassrc_${src}'}' = "xyes"`; then
        AX_GMOCK_SOURCE_CPPFLAGS=$gmock_cxxflags
        AX_GMOCK_SOURCE=$fil
        got="yes";
      fi
    fi
   done
  done
  AS_IF([test "x${got}" = "xyes"], [$2], [$3])
  AC_LANG_POP
])
