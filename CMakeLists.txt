cmake_minimum_required(VERSION 3.15.0)
project(scratch VERSION 0.1.0)

#-----------
# Testing
#-----------

include(CTest)
enable_testing()

#-----------
# Packaging
#-----------

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

#-------------------
# Dependency paths
#-------------------

set(EXT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext)
set(IMGUI_DIR ${EXT_DIR}/cimgui-1.82dock)
set(BACKEND_DIR ${IMGUI_DIR}/imgui/backends)
set(GL3W_DIR ${IMGUI_DIR}/imgui/examples/libs/gl3w)
set(SDL2_DIR ${EXT_DIR}/SDL2-2.0.14)
set(FREETYPE_DIR ${EXT_DIR}/freetype-2.9.1)
set(GLFW_DIR ${EXT_DIR}/glfw-3)

#-------------------

if (WIN32)
    add_definitions("-DIMGUI_IMPL_API=extern \"C\" __declspec\(dllexport\)")
else(WIN32)
    add_definitions("-DIMGUI_IMPL_API=extern \"C\" ")
endif(WIN32)

add_definitions("-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=1")

#-----------------------
# Dependency libraries
#-----------------------

# cimgui static library with freetype support
set(IMGUI_STATIC "yes" CACHE STRING "Build as a static library" FORCE)
set(IMGUI_FREETYPE "yes" CACHE STRING "Build with freetype library" FORCE)
set(FREETYPE_PATH ${FREETYPE_DIR})
add_subdirectory(${IMGUI_DIR})


add_subdirectory(${GLFW_DIR})

add_subdirectory(foundation)

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED PATHS ${SDL2_DIR})

#-----------------------
# IMGUI backend library
#-----------------------

set(BACKEND_SOURCES 
    ${BACKEND_DIR}/imgui_impl_sdl.cpp
    ${BACKEND_DIR}/imgui_impl_glfw.cpp
    ${BACKEND_DIR}/imgui_impl_opengl3.cpp
    ${GL3W_DIR}/GL/gl3w.c)

add_library(backend STATIC ${BACKEND_SOURCES})

target_compile_definitions(backend PRIVATE IMGUI_IMPL_OPENGL_LOADER_GL3W)

target_include_directories(backend PUBLIC 
    ${IMGUI_DIR}/imgui
    ${GL3W_DIR})

target_link_libraries(backend PUBLIC 
    cimgui 
    SDL2::SDL2-static
    glfw
    ${OPENGL_gl_LIBRARY})

#-----------------------
# Main executable
#-----------------------

set(SOURCES main.c)
set_source_files_properties(${SOURCES} PROPERTIES LANGUAGE C)

add_executable( ${PROJECT_NAME} ${SOURCES})

include(cmake/CompileFlags.cmake)
set_c_compile_flags(${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC c_std_11)

target_include_directories(${PROJECT_NAME} PUBLIC ${IMGUI_DIR} ${GL3W_DIR})

target_link_libraries(${PROJECT_NAME} PUBLIC
    foundation
    backend)

#-----------------------
# Resource files
#-----------------------

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION .)
