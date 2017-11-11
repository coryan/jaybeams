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

# ... included from the top-level CMakeLists.txt ...

add_library(jb_clfft SHARED
  error.cpp
  error.hpp
  init.cpp
  init.hpp
  plan.hpp
  )
target_link_libraries(jb_clfft jb_opencl ${CLFFT_LIBRARIES} ${OpenCL_LIBRARY})
# clFFT uses deprecated OpenCL functions in its headers, which results
# in "deprecated declarations" warnings.  We need to silence them to
# support -Werror
CHECK_CXX_COMPILER_FLAG(-Wno-deprecated-declarations COMPILER_SUPPORTS_WNO_D_D)
if(COMPILER_SUPPORTS_WNO_D_D)
  target_compile_options(jb_clfft PUBLIC -Wno-deprecated-declarations)
endif()

set(jb_clfft_unit_tests
  clfft_error_ut
  clfft_plan_ut)
jb_create_unit_test_targets(${jb_clfft_unit_tests})
foreach(fname ${jb_clfft_unit_tests})
  string(REPLACE "/" "_" target ${fname})
  target_link_libraries(${target} jb_clfft)
endforeach()

add_executable(bm_single_fft bm_single_fft.cpp)
target_link_libraries(bm_single_fft jb_clfft jb_testing jb_opencl_testing)
