cmake_minimum_required(VERSION 3.16)
project(NekoExecute)

set(CMAKE_CXX_STANDARD 17)

add_executable(NekoInjector injector.cpp)
target_link_libraries(NekoInjector PRIVATE ntdll)

add_library(NekoExecute SHARED executor.cpp)
target_link_libraries(NekoExecute PRIVATE user32)
