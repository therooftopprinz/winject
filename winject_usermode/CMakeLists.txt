cmake_minimum_required(VERSION 3.10)

project(winject_usermode)

set(CMAKE_VERBOSE_MAKEFILE on)

include(ExternalProject)
include(FetchContent)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)

ExternalProject_Add(googletest
  SOURCE_DIR ../googletest
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/googletest
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=googletest_install
)

ExternalProject_Get_Property(googletest source_dir)
set(GTEST_INCLUDE_DIRS ${source_dir}/googletest/include)
set(GMOCK_INCLUDE_DIRS ${source_dir}/googlemock/include)

ExternalProject_Get_Property(googletest binary_dir)

add_library(gtest UNKNOWN IMPORTED)
set_target_properties(gtest PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${GTEST_INCLUDE_DIRS}
    IMPORTED_LOCATION ${binary_dir}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}gtest.a)
add_dependencies(gtest googletest)

add_library(gtest_main UNKNOWN IMPORTED)
set_target_properties(gtest_main PROPERTIES
    IMPORTED_LOCATION ${binary_dir}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}gtest_main.a)
add_dependencies(gtest_main googletest)

add_library(gmock UNKNOWN IMPORTED)
set_target_properties(gmock PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${GMOCK_INCLUDE_DIRS}
    IMPORTED_LOCATION ${binary_dir}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}gmock.a)
add_dependencies(gmock googletest)

add_library(gmock_main UNKNOWN IMPORTED)
set_target_properties(gmock_main PROPERTIES
    IMPORTED_LOCATION ${binary_dir}/lib/${CMAKE_FIND_LIBRARY_PREFIXES}gmock_main.a)
add_dependencies(gmock_main gtest)

file(GLOB_RECURSE test_src test/*.cpp)
add_executable(test ${test_src})
target_include_directories(test PRIVATE src test . .. ../Logless/src ../cum/src)
target_link_libraries(test PRIVATE pthread gtest gtest_main gmock gmock_main)

if(BUILD_COVERAGE)
    set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
    include(CodeCoverage)
    append_coverage_compiler_flags_to_target(test)
    setup_target_for_coverage_gcovr_html(
        NAME test_coverage
        EXECUTABLE ./test
        DEPENDENCIES test
    )
endif()

file(GLOB_RECURSE winject_usermode_src src/*.cpp)
add_executable(winject_usermode ${winject_usermode_src})
target_include_directories(winject_usermode PRIVATE src . .. ../Logless/src ../cum/src)
target_link_libraries(test PRIVATE pthread)

