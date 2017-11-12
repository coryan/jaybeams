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

file(READ conjugate_and_multiply_kernel.cl KERNEL_SOURCE_CL)
configure_file(conjugate_and_multiply_kernel.cpp.in conjugate_and_multiply_kernel.cpp)
add_library(jb_tde SHARED
        ${CMAKE_CURRENT_BINARY_DIR}/conjugate_and_multiply_kernel.cpp
        conjugate_and_multiply_kernel.hpp
        )
target_link_libraries(jb_tde jb_clfft jb_opencl ${CLFFT_LIBRARY} ${OpenCL_LIBRARY})

set(jb_tde_unit_tests
        ut_conjugate_and_multiply
        ut_reduce_argmax_real
        )
jb_create_unit_test_targets(${jb_tde_unit_tests})
foreach (fname ${jb_tde_unit_tests})
    string(REPLACE "/" "_" target ${fname})
    if ((NOT ${SANITIZER_ENABLED}) AND (NOT ${JB_DISABLE_TESTS}))
        add_test(${target} ${target})
    endif ()
    target_link_libraries(${target} jb_tde)
endforeach ()

add_executable(bm_reduce_argmax_real bm_reduce_argmax_real.cpp)
target_link_libraries(bm_reduce_argmax_real jb_tde jb_clfft jb_opencl_testing jb_opencl jb_testing jb)
