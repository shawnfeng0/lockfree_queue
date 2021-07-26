project(disruptor)
cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 14)

option(DISRUPTOR_BUILD_EXAMPLES "Build examples" OFF)
option(DISRUPTOR_BUILD_TESTS "Build tests" OFF)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)

if (DISRUPTOR_BUILD_TESTS)
    add_subdirectory(tests)
endif ()
