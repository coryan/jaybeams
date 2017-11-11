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

add_library(jb_fftw
  aligned_multi_array.hpp
  aligned_vector.hpp
  allocator.hpp
  cast.hpp
  plan.cpp
  plan.hpp
  tde_result.hpp
  time_delay_estimator.hpp
  time_delay_estimator_many.hpp
  traits.hpp
  )
target_link_libraries(jb_fftw jb ${FFTW3_LIBRARIES})
target_include_directories(jb_fftw PUBLIC ${FFTW3_INCLUDES})

set(jb_fftw_unit_tests
  ut_aligned_multi_array
  ut_allocator
  ut_cast_multi_array
  ut_cast_vector
  ut_plan
  ut_plan_many
  ut_tde_result
  ut_time_delay_estimator
  ut_time_delay_estimator_many
  ut_traits
  )
jb_create_unit_test_targets(${jb_fftw_unit_tests})
foreach(fname ${jb_fftw_unit_tests})
  string(REPLACE "/" "_" target ${fname})
  target_link_libraries(${target} jb_fftw)
endforeach()

add_executable(bm_time_delay_estimator bm_time_delay_estimator.cpp)
target_link_libraries(bm_time_delay_estimator jb_fftw jb_testing)

add_executable(bm_time_delay_estimator_many bm_time_delay_estimator_many.cpp)
target_link_libraries(bm_time_delay_estimator_many jb_fftw jb_testing)
