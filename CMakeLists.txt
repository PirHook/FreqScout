cmake_minimum_required(VERSION 3.10)

project(FreqScout LANGUAGES CXX)

find_package(SDL2 REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE HEADERS "hdr/*.h")
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(FreqScout
    FreqScout.cpp
    ${HEADERS}
    ${SOURCES}
)

target_include_directories(FreqScout PRIVATE hdr)

target_link_libraries(FreqScout PRIVATE SDL2::SDL2)

add_custom_target(link_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_SOURCE_DIR}/compile_commands.json
    COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
)

