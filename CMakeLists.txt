cmake_minimum_required(VERSION 3.29)
project(oculus)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BOOST_INCLUDE_LIBRARIES "process;interprocess")


include(cmake/CPM.cmake)
set(JSON_BuildTests OFF CACHE INTERNAL "")
CPMAddPackage(
    NAME nlohmann_json
    GITHUB_REPOSITORY nlohmann/json
    VERSION 3.9.1)

CPMAddPackage(
    NAME Boost
    URL "https://github.com/boostorg/boost/releases/download/boost-1.83.0/boost-1.83.0.tar.xz"
  )

add_executable(${CMAKE_PROJECT_NAME} main.cpp)

target_link_libraries(${CMAKE_PROJECT_NAME} nlohmann_json::nlohmann_json Boost::process Boost::interprocess)

