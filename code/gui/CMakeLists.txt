cmake_minimum_required(VERSION 3.20.0)

get_directory_property(PARENT_DIR PARENT_DIRECTORY)

message(STATUS "backend parent directory: ${PARENT_DIR}")

#------------------#
# Dependency paths #
#------------------#

set(CODE_DIR ${PARENT_DIR}/code)
set(EXT_DIR ${PARENT_DIR}/ext)
set(IMGUI_DIR ${EXT_DIR}/cimgui-dock/imgui)
set(BACKEND_DIR ${IMGUI_DIR}/backends)
set(FREETYPE_DIR ${EXT_DIR}/freetype-2.9.1)
set(GLFW_DIR ${EXT_DIR}/glfw-3)

#----------------------#
# Dependency libraries #
#----------------------#

# TODO (Matteo): Find a better way to handle out of tree builds
add_subdirectory(${GLFW_DIR} ${CMAKE_BINARY_DIR}/glfw-3)

find_package(OpenGL REQUIRED)
find_package(freetype REQUIRED PATHS ${FREETYPE_DIR})

# NOTE (Matteo): Custom imgui configuration (requires giving access to foundation include directory)
set(GUI_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/gui_config.h)
message(STATUS "backend gui config: ${GUI_CONFIG}")

#-------------#
# Gui library #
#-------------#
set(GUI_NAME gui)

set(GUI_SOURCES 
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
    ${CODE_DIR}/gui/gui.c
    ${CODE_DIR}/gui/gui.cpp)

add_library(${GUI_NAME} STATIC ${GUI_SOURCES})

target_compile_definitions(${GUI_NAME} PUBLIC IMGUI_USER_CONFIG="${GUI_CONFIG}")

target_include_directories(${GUI_NAME} 
    PRIVATE 
        ${IMGUI_DIR}
        ${IMGUI_DIR}/misc/freetype
    PUBLIC
        ${CODE_DIR})

target_link_libraries(${GUI_NAME} PRIVATE freetype)

#-----------------#
# Backend library #
#-----------------#
set(BACKEND_NAME gui_backend)

set(BACKEND_SOURCES 
    ${BACKEND_DIR}/imgui_impl_opengl3.cpp
    ${BACKEND_DIR}/imgui_impl_glfw.cpp
    ${CODE_DIR}/gl/gload.c)

add_library(${BACKEND_NAME} STATIC ${BACKEND_SOURCES})

target_compile_definitions(${BACKEND_NAME} PRIVATE IMGUI_IMPL_OPENGL_LOADER_CUSTOM=<gl/gload.h>)

target_include_directories(${BACKEND_NAME} PRIVATE ${IMGUI_DIR})

target_link_libraries(${BACKEND_NAME} PUBLIC 
    ${GUI_NAME} 
    ${OPENGL_gl_LIBRARY}
    glfw)

#----------------------------#
# Common compilation options #
#----------------------------#

target_compile_features(${GUI_NAME}     PUBLIC c_std_11) 
target_compile_features(${GUI_NAME}     PUBLIC cxx_std_14)
target_compile_features(${BACKEND_NAME} PUBLIC c_std_11) 
target_compile_features(${BACKEND_NAME} PUBLIC cxx_std_14)

include(${PARENT_DIR}/cmake/CompileFlags.cmake)

set_c_compile_flags(${GUI_NAME})
set_c_compile_flags(${BACKEND_NAME})

#-----------------------#
# Unity build all  libs #
#-----------------------#

set_target_properties(${GUI_NAME} ${BACKEND_NAME} PROPERTIES UNITY_BUILD TRUE)
