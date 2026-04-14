cmake_minimum_required(VERSION 3.10)

project(FreqScout LANGUAGES CXX)

find_package(SDL2 REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE HEADERS "hdr/*.h")
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(FreqScout
    FreqScout.cpp
    ${HEADERS}
    ${SOURCES}
)

target_include_directories(FreqScout PRIVATE hdr)

target_link_libraries(FreqScout PRIVATE SDL2::SDL2)

