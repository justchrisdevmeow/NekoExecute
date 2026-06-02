cmake_minimum_required(VERSION 3.16)
project(NekoExecute)

include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.0
)
FetchContent_MakeAvailable(imgui)

add_executable(NekoInjector injector.cpp)
target_link_libraries(NekoInjector PRIVATE user32)

add_library(NekoExecute SHARED executor.cpp)
target_link_libraries(NekoExecute PRIVATE user32 d3d9 dwmapi)
target_link_options(NekoExecute PRIVATE -static-libstdc++ -static-libgcc)

target_sources(NekoExecute PRIVATE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx9.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)

target_include_directories(NekoExecute PRIVATE 
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
