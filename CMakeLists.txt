project(bfx-FIX-test)
cmake_minimum_required(VERSION 3.8)

add_definitions(-DUSE_LIB_CRYPTO)
add_definitions(-DCPPHTTPLIB_ZLIB_SUPPORT)
add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions( --std=c++2a -march=native -Wno-deprecated-declarations -fconcepts )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/
)

add_executable(latency_test latency_test.cpp )
target_link_libraries(latency_test -lpthread)

add_executable(ws_latency_test ws_latency_test.cpp )
target_link_libraries(ws_latency_test -lpthread -lssl -lcrypto -lz)

