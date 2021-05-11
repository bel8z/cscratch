cmake_minimum_required(VERSION 3.20.0)
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

#---------------
# Local scripts
#---------------

include(cmake/CompileFlags.cmake)
include(cmake/Win32Runtime.cmake)

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

option(BACKEND_SDL OFF)

if (WIN32)
    add_definitions("-DIMGUI_IMPL_API=extern \"C\" __declspec\(dllexport\)")
else(WIN32)
    add_definitions("-DIMGUI_IMPL_API=extern \"C\" ")
endif(WIN32)

add_definitions("-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS=1")

#-----------------------
# Resource files
#-----------------------

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if (MSVC)
    foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
        set(OUT_DIR "${CMAKE_BINARY_DIR}/${CONFIG_TYPE}")

        message(STATUS "Output directory (${CONFIG_TYPE}): ${OUT_DIR}")

        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE} "${OUT_DIR}")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE} "${OUT_DIR}")
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE} "${OUT_DIR}")

        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION "${OUT_DIR}")
    endforeach()
else()
    set(OUT_DIR "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}")

    message(STATUS "Output directory: ${OUT_DIR}")

    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUT_DIR}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUT_DIR}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUT_DIR}")
    
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION "${OUT_DIR}")
endif()

#-----------------------
# Dependency libraries
#-----------------------

# cimgui static library with freetype support
set(IMGUI_STATIC "yes" CACHE STRING "Build as a static library" FORCE)
set(IMGUI_FREETYPE "yes" CACHE STRING "Build with freetype library" FORCE)
set(FREETYPE_PATH ${FREETYPE_DIR})
add_subdirectory(${IMGUI_DIR})

set_win32_runtime_static(cimgui)

add_subdirectory(${GLFW_DIR})

add_subdirectory(foundation)

find_package(OpenGL REQUIRED)

#-----------------------
# IMGUI backend library
#-----------------------

set(BACKEND_SOURCES 
    ${BACKEND_DIR}/imgui_impl_opengl3.cpp
    ${GL3W_DIR}/GL/gl3w.c)

if (BACKEND_SDL)
    list(APPEND BACKEND_SOURCES ${BACKEND_DIR}/imgui_impl_sdl.cpp)
else()
    list(APPEND BACKEND_SOURCES ${BACKEND_DIR}/imgui_impl_glfw.cpp)
endif()

add_library(backend STATIC ${BACKEND_SOURCES})

target_compile_definitions(backend PRIVATE IMGUI_IMPL_OPENGL_LOADER_GL3W)

target_include_directories(backend PUBLIC 
    ${IMGUI_DIR}/imgui
    ${GL3W_DIR})

target_link_libraries(backend PUBLIC 
    cimgui 
    ${OPENGL_gl_LIBRARY})

if(BACKEND_SDL)
    find_package(SDL2 REQUIRED PATHS ${SDL2_DIR})
    target_link_libraries(backend PUBLIC SDL2::SDL2-static)
else()
    target_link_libraries(backend PUBLIC glfw)
endif()

set_win32_runtime_static(backend)

#-----------------------
# Main executable
#-----------------------

set(SOURCES 
    tests/test_arena.c)
# set(SOURCES 
#     main.c 
#     image/app.c 
#     image/image.c)
set_source_files_properties(${SOURCES} PROPERTIES LANGUAGE C)

add_executable(${PROJECT_NAME} ${SOURCES})

if(BACKEND_SDL)
    target_compile_definitions(${PROJECT_NAME} PUBLIC "SDL_BACKEND=1")
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC "SDL_BACKEND=0")
endif()

target_compile_features(${PROJECT_NAME} PUBLIC c_std_11)

target_include_directories(${PROJECT_NAME} PUBLIC 
    "."
    ${IMGUI_DIR} 
    ${GL3W_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE
    foundation
    backend)

set_c_compile_flags(${PROJECT_NAME})

# TODO (Matteo): Find the best way to compile everything linking
# only once with the MSVC runtime.
if (MSVC)
    target_compile_options(${PROJECT_NAME} INTERFACE /NODEFAULTLIB)
endif()
