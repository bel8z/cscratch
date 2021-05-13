cmake_minimum_required(VERSION 3.20.0)

get_directory_property(PARENT_DIR PARENT_DIRECTORY)


message(STATUS "backend parent directory: ${PARENT_DIR}")

#-------------------
# Dependency paths
#-------------------

set(CODE_DIR ${PARENT_DIR}/code)
set(EXT_DIR ${PARENT_DIR}/ext)
set(IMGUI_DIR ${EXT_DIR}/cimgui-1.82dock)
set(BACKEND_DIR ${IMGUI_DIR}/imgui/backends)
set(SDL2_DIR ${EXT_DIR}/SDL2-2.0.14)
set(FREETYPE_DIR ${EXT_DIR}/freetype-2.9.1)
set(GLFW_DIR ${EXT_DIR}/glfw-3)

#---------------------
# Compilation options
#---------------------

option(BACKEND_SDL OFF)

if (WIN32)
    add_definitions("-DIMGUI_IMPL_API=extern \"C\" __declspec\(dllexport\)")
else(WIN32)
    add_definitions("-DIMGUI_IMPL_API=extern \"C\" ")
endif(WIN32)

add_definitions("-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=1")

# cimgui static library with freetype support
set(IMGUI_STATIC "yes" CACHE STRING "Build as a static library" FORCE)
set(IMGUI_FREETYPE "yes" CACHE STRING "Build with freetype library" FORCE)
set(FREETYPE_PATH ${FREETYPE_DIR})

#-----------------------
# Dependency libraries
#-----------------------

# TODO (Matteo): Find a better way to handle out of tree builds
add_subdirectory(${IMGUI_DIR} ${CMAKE_BINARY_DIR}/cimgui-1.82dock)
add_subdirectory(${GLFW_DIR}  ${CMAKE_BINARY_DIR}/glfw-3)

find_package(OpenGL REQUIRED)

#-----------------------
# Backend library
#-----------------------

set(BACKEND_SOURCES 
    ${BACKEND_DIR}/imgui_impl_opengl3.cpp
    ${CODE_DIR}/gl/gload.c)

if (BACKEND_SDL)
    list(APPEND BACKEND_SOURCES ${BACKEND_DIR}/imgui_impl_sdl.cpp)
else()
    list(APPEND BACKEND_SOURCES ${BACKEND_DIR}/imgui_impl_glfw.cpp)
endif()

add_library(backend STATIC ${BACKEND_SOURCES})

target_compile_definitions(backend PRIVATE IMGUI_IMPL_OPENGL_LOADER_CUSTOM=<gl/gload.h>)

target_include_directories(backend PUBLIC 
    ${IMGUI_DIR}/imgui
    ${CODE_DIR})

target_include_directories(backend INTERFACE ${IMGUI_DIR})

target_link_libraries(backend PUBLIC 
    cimgui 
    ${OPENGL_gl_LIBRARY})

if(BACKEND_SDL)
    find_package(SDL2 REQUIRED PATHS ${SDL2_DIR})
    target_link_libraries(backend PUBLIC SDL2::SDL2-static)
    target_compile_definitions(backend INTERFACE "SDL_BACKEND=1")
else()
    target_link_libraries(backend PUBLIC glfw)
    target_compile_definitions(backend INTERFACE "SDL_BACKEND=0")
endif()
