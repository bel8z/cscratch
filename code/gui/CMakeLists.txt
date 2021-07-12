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
set(FREETYPE_DIR ${EXT_DIR}/freetype-2.9.1)
set(GLFW_DIR ${EXT_DIR}/glfw-3)

#---------------------
# Compilation options
#---------------------

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
# Gui library
#-----------------------
set(GUI_NAME gui)

set(GUI_SOURCES 
    ${CODE_DIR}/gui/gui.c)

add_library(${GUI_NAME} STATIC ${GUI_SOURCES})

set_source_files_properties(${GUI_SOURCES} PROPERTIES LANGUAGE C)

target_include_directories(${GUI_NAME} PUBLIC 
    ${IMGUI_DIR}/imgui
    ${CODE_DIR})

target_include_directories(${GUI_NAME} INTERFACE ${IMGUI_DIR})

target_link_libraries(${GUI_NAME} PUBLIC cimgui)

target_compile_features(${GUI_NAME} PUBLIC c_std_11)

#-----------------------
# Backend library
#-----------------------
set(BACKEND_NAME gui_backend)

set(BACKEND_SOURCES 
    ${BACKEND_DIR}/imgui_impl_opengl3.cpp
    ${CODE_DIR}/gl/gload.c)

list(APPEND BACKEND_SOURCES ${BACKEND_DIR}/imgui_impl_glfw.cpp)

add_library(${BACKEND_NAME} STATIC ${BACKEND_SOURCES})

target_compile_definitions(${BACKEND_NAME} PRIVATE IMGUI_IMPL_OPENGL_LOADER_CUSTOM=<gl/gload.h>)

target_include_directories(${BACKEND_NAME} INTERFACE ${IMGUI_DIR})

target_link_libraries(${BACKEND_NAME} PUBLIC 
    ${GUI_NAME} 
    ${OPENGL_gl_LIBRARY})

target_link_libraries(${BACKEND_NAME} PUBLIC glfw)
