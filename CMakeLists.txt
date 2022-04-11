project(bfx-FIX-test)
cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions( --std=c++2a -march=native -Wno-deprecated-declarations -fconcepts )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/
)

add_executable(latency_test latency_test.cpp )
target_link_libraries(latency_test -lpthread)

