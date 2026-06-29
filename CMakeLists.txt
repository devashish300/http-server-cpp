cmake_minimum_required(VERSION 3.13)
project(http-server-starter-cpp)
set(CMAKE_CXX_STANDARD 20)

set(SOURCE_FILES server.cpp request_handler.cpp)

find_package(ZLIB REQUIRED)

add_executable(server ${SOURCE_FILES})

target_link_libraries(server pthread ${ZLIB_LIBRARIES})