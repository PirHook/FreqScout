cmake_minimum_required(VERSION 3.10)

project(FreqScout LANGUAGES CXX)

find_package(SDL2 REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/lib/imgui/")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp
)

file(GLOB_RECURSE HEADERS "hdr/*.h")
file(GLOB_RECURSE SOURCES "src/*.cpp")

set(REPROC++ ON CACHE BOOL "Build reproc++" FORCE)
set(REPROC_TEST OFF CACHE BOOL "Build tests" FORCE)
set(REPROC_EXAMPLES OFF CACHE BOOL "Build examples" FORCE)

add_subdirectory(lib/reproc)

add_executable(FreqScout
    FreqScout.cpp
    ${HEADERS}
    ${SOURCES}
    ${IMGUI_SOURCES}
)

target_include_directories(FreqScout PRIVATE
    hdr
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(FreqScout PRIVATE
    SDL2::SDL2
    reproc++
)

