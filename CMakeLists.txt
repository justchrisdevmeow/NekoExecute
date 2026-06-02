cmake_minimum_required(VERSION 3.16)
project(NekoExecute)

add_executable(NekoInjector injector.cpp)
target_link_libraries(NekoInjector PRIVATE user32)

add_library(NekoExecute SHARED executor.cpp)
target_link_libraries(NekoExecute PRIVATE user32 d3d9 dwmapi)
