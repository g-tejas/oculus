cmake_minimum_required(VERSION 3.29)
project(oculus)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BOOST_INCLUDE_LIBRARIES "process")


include(cmake/CPM.cmake)
CPMAddPackage("gh:simdjson/simdjson@3.10.1")

CPMAddPackage(
    NAME Boost
    URL "https://github.com/boostorg/boost/releases/download/boost-1.83.0/boost-1.83.0.tar.xz"
  )

add_executable(${CMAKE_PROJECT_NAME} main.cpp)

target_link_libraries(${CMAKE_PROJECT_NAME} simdjson::simdjson Boost::process)

