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

cmake_minimum_required(VERSION 3.5)

set(PACKAGE_NAME "JayBeams")
set(PACKAGE_VERSION   "0.1")
set(PACKAGE_STRING    "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_TARNAME   "${PACKAGE_NAME}-${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "https://github.com/coryan/jaybeams/issues")

project(Jaybeams CXX C)

# Require C++14 for this project ...
set(CMAKE_CXX_STANDARD 14)
set(CXX_STANDARD_REQUIRED ON)

option(JB_DISABLE_TESTS, "Disable unit and integration tests. Can be useful to avoid installing gmock+gtest." OFF)

# ... the author is paranoid.  Turn on all available warnings
# and turn warnings into errors to stop the build if any warning is
# emitted ...
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG(-Werror COMPILER_SUPPORTS_WERROR)
if(COMPILER_SUPPORTS_WERROR)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()
CHECK_CXX_COMPILER_FLAG(-Wall COMPILER_SUPPORTS_WALL)
if(COMPILER_SUPPORTS_WALL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()
CHECK_CXX_COMPILER_FLAG(/WX COMPILER_SUPPORTS_WX)
if(COMPILER_SUPPORTS_WX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
endif()
CHECK_CXX_COMPILER_FLAG(/W4 COMPILER_SUPPORTS_SWALL)
if(COMPILER_SUPPORTS_SWALL)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall")
endif()

# ... include the functions to compile proto files ...
include(FindProtobuf)

# ... find the protobuf, grpc, and grpc++ libraries using pkg-config ...
include(FindPkgConfig)
pkg_check_modules(PROTOBUF REQUIRED protobuf>=3.0)
pkg_check_modules(GRPCPP REQUIRED grpc++>=1.0)
pkg_check_modules(GRPC REQUIRED grpc>=4.0)
link_directories(${GRPCPP_LIBRARY_DIRS} ${GRPC_LIBRARY_DIRS} ${PROTOBUF_LIBRARY_DIRS})
include_directories(${GRPCPP_INCLUDE_DIRS} ${GRPC_INCLUDE_DIRS} ${PROTOBUF_INCLUDE_DIRS})

# ... we need Boost >= 1.58 because Beast and Boost.Endian require them ...
include(FindBoost)
find_package(Boost 1.58 REQUIRED
  COMPONENTS log log_setup filesystem date_time thread iostreams program_options unit_test_framework)
include_directories(${Boost_INCLUDE_DIRS})

# ... YAML-CPP is a library to parse YAML files, JayBeams uses such files for all runtime configuration ...
find_package(yaml-cpp REQUIRED)

# ... Beast (https://github.com/boostorg/beast) is a library for writing HTTP clients and servers ...
option(Boost_USE_STATIC_LIBS "Disable static Boost libraries in Beast" OFF)
option(Beast_BUILD_EXAMPLES "Disable examples in Beast" OFF)
option(Beast_BUILD_TESTS "Disable tests in Beast" OFF)
add_subdirectory(ext/Beast)
include_directories(${PROJECT_SOURCE_DIR}/ext/Beast/include)

# ... if set, enable code coverage builds for g++, and create a "test_coverage" target to collect the coverage using
# lcov.  My attempts at using the gcovr cobertura output failed miserably, so I am sticking to what I know ...
set(COVERAGE false CACHE BOOL "Enable code coverage builds.")
if(${COVERAGE})
    include(cmake/CodeCoverage.cmake)
    include(ProcessorCount)
    ProcessorCount(NCPUS)
    set(ctest_parallelism "")
    if(NOT N EQUAL 0)
        set(ctest_parallelism "-j ${NCPUS}")
    endif()
    APPEND_COVERAGE_COMPILER_FLAGS()
    SETUP_TARGET_FOR_COVERAGE(
            NAME test_coverage
            EXECUTABLE ctest ${ctest_parallelism}
    )
endif(${COVERAGE})

# ... we need the -I flags so we can find header files using the installed path for JayBeams headers ...
include_directories("${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")

# Compile things into shared libraries, apparently it is easy to instruct cmake(1) to generate a static library even
# if we only describe how to create a shared lib.
add_library(jb SHARED
  jb/as_hhmmss.cpp
  jb/as_hhmmss.hpp
  jb/assert_throw.cpp
  jb/assert_throw.hpp
  jb/book_depth_statistics.cpp
  jb/book_depth_statistics.hpp
  jb/complex_traits.hpp
  jb/config_attribute.hpp
  jb/config_files_location.cpp
  jb/config_files_location.hpp
  jb/config_object.cpp
  jb/config_object.hpp
  jb/config_recurse.hpp
  jb/convert_cpu_set.hpp
  jb/convert_severity_level.hpp
  jb/cpu_set.cpp
  jb/cpu_set.hpp
  jb/detail/array_traits.hpp
  jb/detail/os_error.cpp
  jb/detail/os_error.hpp
  jb/detail/reconfigure_thread.cpp
  jb/detail/reconfigure_thread.hpp
  jb/detail/thread_setup_wrapper.hpp
  jb/event_rate_estimator.hpp
  jb/event_rate_histogram.hpp
  jb/explicit_cuts_binning.hpp
  jb/feed_error.hpp
  jb/fileio.cpp
  jb/fileio.hpp
  jb/filetype.cpp
  jb/filetype.hpp
  jb/fixed_string.hpp
  jb/histogram.hpp
  jb/histogram_summary.cpp
  jb/histogram_summary.hpp
  jb/integer_range_binning.hpp
  jb/launch_thread.hpp
  jb/log.cpp
  jb/log.hpp
  jb/merge_yaml.cpp
  jb/merge_yaml.hpp
  jb/offline_feed_statistics.cpp
  jb/offline_feed_statistics.hpp
  jb/p2ceil.hpp
  jb/severity_level.cpp
  jb/severity_level.hpp
  jb/strtonum.hpp
  jb/thread_config.cpp
  jb/thread_config.hpp
  jb/usage.hpp
  )
target_compile_definitions(jb PUBLIC -DBOOST_LOG_DYN_LINK)
target_link_libraries(jb Boost::log Boost::program_options Boost::iostreams yaml-cpp)
# ... the jb library unit tests, we setup the targets and dependencies further down using a loop ...
set(jb_unit_tests
  jb_ut_as_hhmmss
  jb_ut_assert_throw
  jb_ut_book_depth_statistics
  jb_ut_config_files_location
  jb_ut_config_object
  jb_ut_config_object_vector
  jb_ut_cpu_set
  jb_ut_event_rate_estimator
  jb_ut_event_rate_histogram
  jb_ut_explicit_cuts_binning
  jb_ut_fileio
  jb_ut_filetype
  jb_ut_fixed_string
  jb_ut_histogram
  jb_ut_histogram_summary
  jb_ut_integer_range_binning
  jb_ut_launch_thread
  jb_ut_logging
  jb_ut_merge_yaml
  jb_ut_offline_feed_statistics
  jb_ut_p2ceil
  jb_ut_severity_level
  jb_ut_strtonum
  jb_ut_thread_config
  )
add_executable(examples_configuration examples/configuration.cpp)
target_link_libraries(examples_configuration jb)

add_custom_command(
  OUTPUT compile_info.cpp
  COMMAND ${PROJECT_SOURCE_DIR}/tools/gen_compile_info.sh
      ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} >${PROJECT_BINARY_DIR}/compile_info.cpp
  )

add_library(jb_testing SHARED
  jb/testing/check_close_enough.hpp
  ${PROJECT_BINARY_DIR}/compile_info.cpp
  jb/testing/compile_info.hpp
  jb/testing/create_square_timeseries.hpp
  jb/testing/create_triangle_timeseries.hpp
  jb/testing/delay_timeseries.hpp
  jb/testing/future_status.hpp
  jb/testing/initialize_mersenne_twister.hpp
  jb/testing/microbenchmark.hpp
  jb/testing/microbenchmark_base.cpp
  jb/testing/microbenchmark_base.hpp
  jb/testing/microbenchmark_config.cpp
  jb/testing/microbenchmark_config.hpp
  jb/testing/microbenchmark_group_main.cpp
  jb/testing/microbenchmark_group_main.hpp
  jb/testing/sum_square.hpp
  )
target_link_libraries(jb_testing jb Boost::log Boost::program_options Boost::iostreams yaml-cpp)
set(jb_testing_unit_tests
  jb_testing_future_status_ut
  jb_testing_ut_check_close_enough
  jb_testing_ut_create_square_timeseries
  jb_testing_ut_create_triangle_timeseries
  jb_testing_ut_delay_timeseries
  jb_testing_ut_microbenchmark_config
  )
add_executable(jb_testing_check_mt19937_initializer jb/testing/check_mt19937_initializer.cpp)
target_link_libraries(jb_testing_check_mt19937_initializer jb_testing jb)
add_executable(jb_testing_check_random_device jb/testing/check_random_device.cpp)
target_link_libraries(jb_testing_check_random_device jb_testing jb)

add_executable(jb_bm_clocks jb/bm_clocks.cpp)
target_link_libraries(jb_bm_clocks jb_testing jb)

add_library(jb_gmock SHARED
  jb/gmock/init.hpp
  )
target_link_libraries(jb_gmock gmock gtest)

#add_library(jb_itch5 SHARED
#  )

add_library(jb_ehs SHARED
  jb/ehs/acceptor.cpp
  jb/ehs/acceptor.hpp
  jb/ehs/base_types.hpp
  jb/ehs/connection.cpp
  jb/ehs/connection.hpp
  jb/ehs/request_dispatcher.cpp
  jb/ehs/request_dispatcher.hpp
  )
target_link_libraries(jb_ehs jb Boost::log Boost::program_options Boost::iostreams yaml-cpp)
set(jb_ehs_unit_tests
  jb_ehs_ut_acceptor
  jb_ehs_ut_request_dispatcher)

add_library(jb_pitch2 SHARED
  jb/pitch2/add_order_message.cpp
  jb/pitch2/add_order_message.hpp
  jb/pitch2/auction_update_message.cpp
  jb/pitch2/auction_update_message.hpp
  jb/pitch2/base_add_order_message.hpp
  jb/pitch2/base_add_order_message_streaming.hpp
  jb/pitch2/delete_order_message.cpp
  jb/pitch2/delete_order_message.hpp
  jb/pitch2/expanded_add_order_message.cpp
  jb/pitch2/expanded_add_order_message.hpp
  jb/pitch2/modify_message.cpp
  jb/pitch2/modify_message.hpp
  jb/pitch2/order_executed_message.cpp
  jb/pitch2/order_executed_message.hpp
  jb/pitch2/order_executed_price_message.cpp
  jb/pitch2/order_executed_price_message.hpp
  jb/pitch2/reduce_size_message.cpp
  jb/pitch2/reduce_size_message.hpp
  jb/pitch2/short_add_order_message.cpp
  jb/pitch2/short_add_order_message.hpp
  jb/pitch2/time_message.cpp
  jb/pitch2/time_message.hpp
  jb/pitch2/unit_clear_message.cpp
  jb/pitch2/unit_clear_message.hpp
  )
set(jb_pitch2_unit_tests
  jb_pitch2_ut_add_order_message
  jb_pitch2_ut_auction_update_message
  jb_pitch2_ut_delete_order_message
  jb_pitch2_ut_expanded_add_order_message
  jb_pitch2_ut_modify_message
  jb_pitch2_ut_order_executed_message
  jb_pitch2_ut_order_executed_price_message
  jb_pitch2_ut_reduce_size_message
  jb_pitch2_ut_short_add_order_message
  jb_pitch2_ut_time_message
  jb_pitch2_ut_unit_clear_message
  )
target_link_libraries(jb_pitch2 jb Boost::log Boost::program_options Boost::iostreams yaml-cpp)

set(bin_SCRIPTS tools/benchmark_common.sh)
set(bin_PROGRAMS
  jb/itch5/mold2inside
  jb/itch5/moldfeedhandler
  jb/itch5/moldreplay
  jb/testing/show_compile_info
  tools/itch5bookdepth
  tools/itch5eventdepth
  tools/itch5inside
  tools/itch5moldreplay
  tools/itch5stats
  tools/itch5trades
  tools/moldheartbeat
  )

include(cmake/CompileProtos.cmake)
set(PROTOBUF_IMPORT_DIRS "${PROJECT_SOURCE_DIR}/ext/googleapis"
  "${PROJECT_SOURCE_DIR}/ext/gogoproto"
  "${PROJECT_SOURCE_DIR}/ext"
  )
PROTOBUF_GENERATE_CPP(PROTO_SOURCES PROTO_HDRS
  ${PROJECT_SOURCE_DIR}/ext/googleapis/google/api/http.proto
  ${PROJECT_SOURCE_DIR}/ext/googleapis/google/api/annotations.proto
  ${PROJECT_SOURCE_DIR}/ext/gogoproto/gogoproto/gogo.proto
  ${PROJECT_SOURCE_DIR}/ext/etcd/mvcc/mvccpb/kv.proto
  ${PROJECT_SOURCE_DIR}/ext/etcd/auth/authpb/auth.proto
  ${PROJECT_SOURCE_DIR}/ext/etcd/etcdserver/etcdserverpb/rpc.proto
  )
GRPC_GENERATE_CPP(GRPCPP_SOURCES GRPCPP_HDRS GRPCPP_MOCKS
  ${PROJECT_SOURCE_DIR}/ext/etcd/etcdserver/etcdserverpb/rpc.proto
  )
add_library(jb_etcd SHARED
  ${PROTO_SOURCES} ${PROTO_HDRS}
  ${GRPCPP_SOURCES} ${GRPCPP_HEADERS}
  )
target_link_libraries(jb_etcd ${GRPCPP_LIBRARIES} ${GRPC_LIBRARIES} ${PROTOBUF_LIBRARIES})

# ... determine if libraries are installed in the lib64 directory ...
get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
if ("${LIB64}" STREQUAL "TRUE")
    set(LIBSUFFIX 64)
else()
    set(LIBSUFFIX "")
endif()
set(INSTALL_LIB_DIR lib${LIBSUFFIX} CACHE PATH "Installation directory for libraries")

# ... generate a pkg-config for Jaybeams ...
SET(PKG_CONFIG_REQUIRES "grpc++ grpc protobuf")
SET(PKG_CONFIG_LIBDIR "\${prefix}/lib")
SET(PKG_CONFIG_INCLUDEDIR "\${prefix}/include")
SET(PKG_CONFIG_LIBS "-L\${libdir} -ljb")
SET(PKG_CONFIG_CFLAGS "-I\${includedir}")
#CONFIGURE_FILE(
#        "${CMAKE_CURRENT_SOURCE_DIR}/templates/pkg-config.pc.in"
#        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc")

# ... define the install rules ...
install(TARGETS jb
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION ${INSTALL_LIB_DIR})
install(DIRECTORY jb/ DESTINATION include/jb FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY ${CMAKE_BINARY_DIR}/etcd/ DESTINATION include/etcd FILES_MATCHING PATTERN "*.h")
install(DIRECTORY ${CMAKE_BINARY_DIR}/gogoproto/ DESTINATION include/gogoproto FILES_MATCHING PATTERN "*.h")
install(DIRECTORY ${CMAKE_BINARY_DIR}/google/ DESTINATION include/google FILES_MATCHING PATTERN "*.h")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc" DESTINATION lib/pkgconfig)

include(GNUInstallDirs)
add_definitions(-DJB_SYSCONFDIR="${CMAKE_INSTALL_FULL_SYSCONFDIR}" -DJB_BINDIR="${CMAKE_INSTALL_FULL_BINDIR}")

if(NOT JB_DISABLE_TESTS)
  # ... turn on testing support in CMake ...
  enable_testing()

  # ... external dependencies and packages ...
  add_subdirectory(ext/googletest/googlemock)

  # ... add the right libraries to all the unit tests, and also turn them into tests ...
  set(ut_sources)
  foreach(target ${jb_unit_tests} ${jb_testing_unit_tests} ${jb_pitch2_unit_tests} ${jb_ehs_unit_tests})
    string(REGEX MATCH ".*_ut$" is_ut ${target})
    if(NOT is_ut)
      string(REGEX MATCH ".*_ut_.*" is_ut ${target})
    endif()
    if(is_ut)
      set(fname ${target})
      string(REGEX REPLACE "^jb_pitch2_" "jb/pitch2/" fname ${fname})
      string(REGEX REPLACE "^jb_ehs_" "jb/ehs/" fname ${fname})
      string(REGEX REPLACE "^jb_testing_" "jb/testing/" fname ${fname})
      string(REGEX REPLACE "^jb_detail_" "jb/detail/" fname ${fname})
      string(REGEX REPLACE "^jb_" "jb/" fname ${fname})
      add_executable(${target} ${fname}.cpp)
      get_target_property(tname ${target} NAME)
      add_test(${tname} ${target})
      target_compile_definitions(${target} PUBLIC -DBOOST_TEST_MAIN -DBOOST_TEST_DYN_LINK -DBOOST_TEST_MODULE=${target})
      target_link_libraries(${target} jb Boost::unit_test_framework)
    endif()
  endforeach()
  foreach(target ${jb_testing_unit_tests})
      message(STATUS ${target})
      target_link_libraries(${target} jb_testing)
  endforeach()
  foreach(target ${jb_pitch2_unit_tests})
      message(STATUS ${target})
      target_link_libraries(${target} jb_pitch2)
  endforeach()
  foreach(target ${jb_ehs_unit_tests})
      message(STATUS ${target})
      target_link_libraries(${target} jb_ehs)
  endforeach()
  target_link_libraries(jb_ut_config_files_location jb jb_gmock Boost::unit_test_framework)
endif(NOT JB_DISABLE_TESTS)
