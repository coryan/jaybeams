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

file(READ generic_reduce_program.cl GENERIC_REDUCE_CL)
configure_file(generic_reduce_program.cpp.in generic_reduce_program.cpp)

add_library(jb_opencl SHARED
  build_simple_kernel.cpp
  build_simple_kernel.hpp
  config.cpp
  config.hpp
  device_selector.cpp
  device_selector.hpp
  ${CMAKE_CURRENT_BINARY_DIR}/generic_reduce_program.cpp
  generic_reduce_program.hpp
  )
target_link_libraries(jb_opencl jb ${OpenCL_LIBRARY})
# Boost.Compute uses attributes where it should not, which results in
# "ignored attributes" warnings, we need to silence them to support
# -Werror
CHECK_CXX_COMPILER_FLAG(-Wno-ignored-attributes COMPILER_SUPPORTS_WNO_I_A)
if(COMPILER_SUPPORTS_WNO_I_A)
  target_compile_options(jb_opencl PUBLIC -Wno-ignored-attributes)
endif()

add_library(jb_opencl_testing SHARED
  microbenchmark_config.cpp
  microbenchmark_config.hpp
  )
target_link_libraries(jb_opencl_testing jb_opencl jb ${OpenCL_LIBRARY})

set(jb_opencl_unit_tests
  ut_build_simple_kernel
  ut_config
  ut_device_selector
  ut_generic_reduce
  )
jb_create_unit_test_targets(${jb_opencl_unit_tests})
foreach(fname ${jb_opencl_unit_tests})
  string(REPLACE "/" "_" target ${fname})
  target_link_libraries(${target} jb_opencl)
endforeach()

add_executable(show_device show_device.cpp)
target_link_libraries(show_device jb_opencl)

add_executable(bm_copy_buffer bm_copy_buffer.cpp)
target_link_libraries(bm_copy_buffer jb_opencl_testing jb_opencl jb_testing)

add_executable(bm_generic_reduce bm_generic_reduce.cpp)
target_link_libraries(bm_generic_reduce jb_opencl_testing jb_opencl jb_testing)

add_executable(bm_launch_kernel bm_launch_kernel.cpp)
target_link_libraries(bm_launch_kernel jb_opencl_testing jb_opencl jb_testing)

