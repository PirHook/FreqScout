cmake_minimum_required(VERSION 3.10)

project(FreqScout LANGUAGES CXX)

find_package(SDL2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFTW REQUIRED fftw3f)
pkg_check_modules(RTLSDR REQUIRED librtlsdr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(REPROC++ ON CACHE BOOL "Build reproc++" FORCE)
set(REPROC_TEST OFF CACHE BOOL "Build tests" FORCE)
set(REPROC_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)

add_subdirectory(lib/imgui)
add_subdirectory(lib/reproc)

file(GLOB_RECURSE HEADERS "hdr/*.h")
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(FreqScout
    FreqScout.cpp
    ${HEADERS}
    ${SOURCES}
)

target_include_directories(FreqScout PRIVATE
    hdr
    ${FFTW_INCLUDE_DIRS}
    ${RTLSDR_INCLUDE_DIRS}
)

target_link_libraries(FreqScout PRIVATE
    SDL2::SDL2
    reproc++
    imgui
    ${RTLSDR_LIBRARIES}
    ${FFTW_LIBRARIES}
    atomic
)

