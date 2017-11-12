#   Copyright 2017 Carlos O'Ryan
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

if(OpenCL_FOUND)
  if(CLFFT_INCLUDES)
    # Already in cache, be silent
    set(CLFFT_FIND_QUIETLY TRUE)
  endif(CLFFT_INCLUDES)

  find_path(_CLFFT_ROOT_DIR
    NAMES include/clFFT.h
    HINTS /usr/local /opt /opt/local ${CLFFT_ROOT}
    DOC "CLFFT installation directory.")
  find_path(
    CLFFT_INCLUDES clFFT.h
    HINTS ${_CLFFT_ROOT_DIR}/include)
  find_library(CLFFT_LIBRARIES NAMES clFFT
    HINTS ${_CLFFT_ROOT_DIR}/lib)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(CLFFT DEFAULT_MSG CLFFT_LIBRARIES CLFFT_INCLUDES)
  mark_as_advanced(CLFFT_LIBRARIES CLFFT_INCLUDES)
endif(OpenCL_FOUND)

