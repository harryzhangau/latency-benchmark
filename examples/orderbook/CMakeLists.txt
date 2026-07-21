cmake_minimum_required(VERSION 3.28)
project(OrderbookBenchmark CXX)

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
    argparse
    GIT_REPOSITORY https://github.com/p-ranav/argparse.git
    GIT_TAG        v3.2
)

FetchContent_MakeAvailable(argparse)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.17.0
)

FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.17.0
)

FetchContent_MakeAvailable(googletest)

add_executable(orderbook_benchmark 
     ${CMAKE_CURRENT_SOURCE_DIR}/orderbook_benchmark.cpp
)

target_link_libraries(orderbook_benchmark latencybenchmark spdlog::spdlog argparse)
target_include_directories(orderbook_benchmark PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


add_executable(orderbook_benchmark_test)
target_sources(orderbook_benchmark_test
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/TestMessageBusDevice.cpp
)

target_include_directories(orderbook_benchmark_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(orderbook_benchmark_test latencybenchmark GTest::gtest_main)

add_test(NAME OrderbookBenchmarkTest COMMAND orderbook_benchmark_test)