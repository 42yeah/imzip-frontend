cmake_minimum_required(VERSION 3.5)
project(imzip)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
# Linux -pthread shenanigans
if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

#=== LIBRARY: ImGui
add_library(imgui STATIC
    imgui/imgui.cpp
    imgui/imgui.h
    imgui/imgui_widgets.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_demo.cpp)
target_include_directories(imgui INTERFACE imgui)

#=== LIBRARY: sokol
# add headers to the the file list because they are useful to have in IDEs
set(SOKOL_HEADERS
    sokol/sokol_gfx.h
    sokol/sokol_app.h
    sokol/sokol_imgui.h
    sokol/sokol_glue.h)
if(CMAKE_SYSTEM_NAME STREQUAL Darwin)
    add_library(sokol STATIC sokol/sokol.cpp ${SOKOL_HEADERS})
    target_compile_options(sokol PRIVATE -x objective-c)
    target_link_libraries(sokol PUBLIC
        "-framework QuartzCore"
        "-framework Cocoa"
        "-framework MetalKit"
        "-framework Metal")
else()
    add_library(sokol STATIC sokol/sokol.cpp ${SOKOL_HEADERS})
    if (CMAKE_SYSTEM_NAME STREQUAL Linux)
        target_link_libraries(sokol INTERFACE X11 Xi Xcursor GL dl m)
        target_link_libraries(sokol PUBLIC Threads::Threads)
    endif()
endif()
target_link_libraries(sokol PUBLIC imgui)
target_include_directories(sokol INTERFACE sokol)

#=== LIBRARY: miniz
add_library(miniz miniz/miniz.c)
target_include_directories(miniz INTERFACE miniz)

#=== EXECUTABLE: imzip
set(IMZIP_FRONTEND_SRC main.cpp image.cpp)
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
    add_executable(imzip WIN32 ${IMZIP_FRONTEND_SRC})
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT imzip)
else()
    add_executable(imzip ${IMZIP_FRONTEND_SRC})
endif()
target_link_libraries(imzip sokol miniz)

# Emscripten-specific linker options
if (CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    # use our own minimal shell.html
    target_link_options(imzip PRIVATE -sEXPORTED_FUNCTIONS=_main,_images_selected)
    # File system support: https://emscripten.org/docs/api_reference/Filesystem-API.html#including-file-system-support
    target_link_options(imzip PRIVATE -sWASMFS -sFORCE_FILESYSTEM -sEXPORTED_RUNTIME_METHODS=['FS'] -sALLOW_MEMORY_GROWTH)
    target_link_options(imzip PRIVATE --shell-file ${PROJECT_SOURCE_DIR}/shell.html)
    # link with WebGL2
    target_link_options(imzip PRIVATE -sUSE_WEBGL2=1)
    # WASM+JS size optimizations
    target_link_options(imzip PRIVATE -sASSERTIONS -sMALLOC=emmalloc --closure=1)
endif()

# explicitly strip dead code
if (CMAKE_C_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_SYSTEM_NAME STREQUAL Emscripten)
    target_link_options(imzip PRIVATE LINKER:-dead_strip)
endif()

