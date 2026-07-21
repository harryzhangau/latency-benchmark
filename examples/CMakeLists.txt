cmake_minimum_required(VERSION 3.28)
project(LatencyBenchmarkExamples CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS -Wno-interference-size)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g -O0)
    add_link_options(-fsanitize=address)
endif()

add_compile_options(-fconcepts-diagnostics-depth=3)

add_subdirectory(orderbook)
