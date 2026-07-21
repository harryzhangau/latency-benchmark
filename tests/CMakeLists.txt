cmake_minimum_required(VERSION 3.28)
project(LatencyBenchmarkTests CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS -Wno-interference-size)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g -O0)
    add_link_options(-fsanitize=address)
endif()

add_compile_options(-fconcepts-diagnostics-depth=3)

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0
)

FetchContent_MakeAvailable(googletest)

add_executable(latency_benchmark_test)

target_sources(latency_benchmark_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/TestExchangeOrderBook.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/TestHashMap.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/TestObjectList.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/TestObjectPool.cpp
)

target_include_directories(latency_benchmark_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(latency_benchmark_test spdlog::spdlog latencybenchmark GTest::gtest_main)

add_test(NAME LatencyBenchmarkTest COMMAND latency_benchmark_test)