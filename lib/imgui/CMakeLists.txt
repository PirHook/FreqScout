cmake_minimum_required(VERSION 3.10)

project(FreqScout LANGUAGES CXX)

find_package(SDL2 REQUIRED)

add_library(imgui STATIC
    imgui.cpp
    imgui_draw.cpp
    imgui_widgets.cpp
    imgui_tables.cpp
    imgui_demo.cpp

    backends/imgui_impl_sdl2.cpp
    backends/imgui_impl_sdlrenderer2.cpp
  )

target_include_directories(imgui PUBLIC
    .
    backends
)

target_link_libraries(imgui PRIVATE SDL2::SDL2)
