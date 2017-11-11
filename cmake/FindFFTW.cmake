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

if(FFTW3_INCLUDES)
  # Already in cache, be silent
  set(FFTW3_FIND_QUIETLY TRUE)
endif(FFTW3_INCLUDES)

find_path(FFTW3_INCLUDES fftw3.h)
find_library(FFTW3_LIB NAMES fftw3)
find_library(FFTW3L_LIB NAMES fftw3l)
find_library(FFTW3F_LIB NAMES fftw3f)
set(FFTW3_LIBRARIES ${FFTW3_LIB} ${FFTW3L_LIB} ${FFTW3F_LIB})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFTW3 DEFAULT_MSG FFTW3_LIBRARIES FFTW3_INCLUDES)
mark_as_advanced(FFTW3_LIBRARIES FFTW3_INCLUDES)
