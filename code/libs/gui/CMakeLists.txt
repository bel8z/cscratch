cmake_minimum_required(VERSION 3.20.0)

get_directory_property(PARENT_DIR PARENT_DIRECTORY)

message(STATUS "backend parent directory: ${PARENT_DIR}")

#------------------#
# Dependency paths #
#------------------#

set(IMGUI_DIR ${EXT_DIR}/imgui)
set(IMPLOT_DIR ${EXT_DIR}/implot-v0.13)
set(BACKEND_DIR ${IMGUI_DIR}/backends)
set(FREETYPE_DIR ${EXT_DIR}/freetype-2.9.1)
set(GLFW_DIR ${EXT_DIR}/glfw-3.3.6)

#----------------------#
# Dependency libraries #
#----------------------#

# TODO (Matteo): Find a better way to handle out of tree builds

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs" FORCE)
add_subdirectory(${GLFW_DIR} ${CMAKE_BINARY_DIR}/glfw-3.3.6)

find_package(freetype REQUIRED PATHS ${FREETYPE_DIR})

# NOTE (Matteo): Custom imgui configuration (requires giving access to foundation include directory)
set(GUI_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/gui_config.h)
message(STATUS "backend gui config: ${GUI_CONFIG}")

#-------------#
# Gui library #
#-------------#
set(GUI_NAME gui)

file(GLOB GUI_SOURCES 
    ${IMGUI_DIR}/*.cpp
    ${IMPLOT_DIR}/*.cpp
    ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
    ${LIBS_DIR}/gui/gui.c
    ${LIBS_DIR}/gui/gui.cpp)

add_library(${GUI_NAME} STATIC ${GUI_SOURCES})

target_compile_definitions(${GUI_NAME} PUBLIC IMGUI_USER_CONFIG="${GUI_CONFIG}")
target_link_libraries(${GUI_NAME} PRIVATE freetype)
target_include_directories(${GUI_NAME} 
    PRIVATE 
        ${IMGUI_DIR}
        ${IMPLOT_DIR}
        ${IMGUI_DIR}/misc/freetype
        ${EXT_DIR}/stb
    PUBLIC
        ${LIBS_DIR})

#----------------#
# Window library #
#----------------#
set(WIN_NAME gui_win)
set(WIN_SOURCES ${LIBS_DIR}/gui/win.cpp)

add_library(${WIN_NAME} STATIC ${WIN_SOURCES})

target_include_directories(${WIN_NAME} 
    PRIVATE 
        ${IMGUI_DIR} 
        ${IMPLOT_DIR} 
        ${IMGUI_DIR}/misc/freetype)
target_link_libraries(${WIN_NAME} PUBLIC ${GUI_NAME} PRIVATE glfw)

#------------------------#
# OpenGL backend library #
#------------------------#
set(BACKEND_NAME gui_gl3)

find_package(OpenGL REQUIRED)

set(BACKEND_SOURCES 
    ${LIBS_DIR}/gui/gui_backend_gl3.cpp
    ${LIBS_DIR}/gl/gl_api.c)

add_library(${BACKEND_NAME} STATIC ${BACKEND_SOURCES})

target_include_directories(${BACKEND_NAME} PRIVATE ${IMGUI_DIR} ${IMPLOT_DIR})
target_link_libraries(${BACKEND_NAME} PUBLIC 
    ${WIN_NAME} 
    ${OPENGL_gl_LIBRARY})

#----------------------------#
# Common compilation options #
#----------------------------#

target_compile_features(${GUI_NAME}     PUBLIC c_std_11) 
target_compile_features(${GUI_NAME}     PUBLIC cxx_std_14)
target_compile_features(${WIN_NAME}     PUBLIC c_std_11) 
target_compile_features(${WIN_NAME}     PUBLIC cxx_std_14)
target_compile_features(${BACKEND_NAME} PUBLIC c_std_11) 
target_compile_features(${BACKEND_NAME} PUBLIC cxx_std_14)

include(${PARENT_DIR}/cmake/CompileFlags.cmake)

set_c_compile_flags(${GUI_NAME})
set_c_compile_flags(${BACKEND_NAME})
set_c_compile_flags(${WIN_NAME})
