cmake_minimum_required(VERSION 3.20.0)

#---------------#
# Local scripts #
#---------------#

include(cmake/CompileFlags.cmake)
include(cmake/SanitizeProfile.cmake)

#-----------------------------#
# Configure sanitizer profile #
#-----------------------------#

configure_sanitize_profile()

#----------------#
# Global options #
#----------------#

# Force linking against MSVC runtime static library for all targets
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

#---------------------#
# Project declaration #
#---------------------#

project(scratch VERSION 0.1.0)

#-----------#
# Packaging #
#-----------#

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

#------------------#
# Dependency paths #
#------------------#

set(EXT_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/ext)
set(CODE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/code)
set(LIBS_DIR  ${CODE_DIR}/libs)
set(APPS_DIR  ${CODE_DIR}/apps)
set(TESTS_DIR ${CODE_DIR}/tests)

#----------------------#
# Dependency libraries #
#----------------------#

add_subdirectory(${LIBS_DIR}/foundation)

#-----------------------#
# Common resource files #
#-----------------------#

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

#-------------#
# Gui library #
#-------------#

set(GUI_NAME gui)
set(IMGUI_DIR ${EXT_DIR}/imgui)
set(IMPLOT_DIR ${EXT_DIR}/implot)
set(FREETYPE_DIR ${EXT_DIR}/freetype-2.9.1)

find_package(freetype REQUIRED PATHS ${FREETYPE_DIR})

# NOTE (Matteo): Custom imgui configuration (requires giving access to foundation include directory)
set(GUI_CONFIG ${LIBS_DIR}/gui/gui_config.h)
message(STATUS "Gui config: ${GUI_CONFIG}")

file(GLOB GUI_SOURCES 
    ${IMGUI_DIR}/*.cpp
    ${IMPLOT_DIR}/*.cpp
    ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
    ${LIBS_DIR}/gui/gui.c
    ${LIBS_DIR}/gui/gui.cpp)

add_library(${GUI_NAME} STATIC ${GUI_SOURCES})

target_compile_features(${GUI_NAME}    PUBLIC c_std_11) 
target_compile_features(${GUI_NAME}    PUBLIC cxx_std_14)
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

#--------------#
# Applications #
#--------------#

find_package(OpenGL REQUIRED)

file(GLOB APPS RELATIVE ${APPS_DIR} ${APPS_DIR}/*)
foreach(APP ${APPS})
    if(IS_DIRECTORY ${APPS_DIR}/${APP})
        # add_subdirectory(${APPS_DIR}/${APP})

        message(STATUS "Configuring ${APP}")

        set(EXE_NAME ${APP})      
        set(EXE_RESOURCES)
        file(GLOB_RECURSE EXE_RESOURCES ${APPS_DIR}/${APP}/*.rc)
        set_source_files_properties(${EXE_RESOURCES} PROPERTIES LANGUAGE RC)
        add_executable(${EXE_NAME} WIN32 
            ${CODE_DIR}/win32_gui.c
            ${CODE_DIR}/win32_gui_backend.cpp
            ${LIBS_DIR}/gl/gl_api.c
            ${EXE_RESOURCES})

        # Even if Win32 functions should be called explicitly and not relying on
        # macros, I find it safer to enforce the Unicode (WCHAR) variants
        target_compile_definitions(${EXE_NAME} PUBLIC UNICODE=1 _UNICODE=1)

        # NOTE (Matteo): _lib suffix added to prevent conflicts with .pdb files
        set(LIB_NAME ${APP}_lib) 
        set(LIB_SOURCES)
        file(GLOB_RECURSE LIB_SOURCES ${APPS_DIR}/${APP}/*.c)
        list(APPEND LIB_SOURCES ${LIBS_DIR}/gl/gl_api.c)
        add_library(${LIB_NAME} SHARED ${LIB_SOURCES})

        target_include_directories(${EXE_NAME} 
            PUBLIC 
                ${LIBS_DIR}
            PRIVATE 
                ${IMGUI_DIR} 
                ${IMPLOT_DIR} 
                ${IMGUI_DIR}/misc/freetype)

        target_include_directories(${LIB_NAME} 
            PUBLIC 
                ${LIBS_DIR} 
                ${EXT_DIR}/stb)

        target_link_libraries(${EXE_NAME} PRIVATE foundation ${GUI_NAME} ${OPENGL_gl_LIBRARY} dwmapi)
        target_link_libraries(${LIB_NAME} PRIVATE foundation ${GUI_NAME} ${OPENGL_gl_LIBRARY}) 

        target_compile_features(${EXE_NAME} PUBLIC c_std_11)
        target_compile_features(${EXE_NAME} PUBLIC cxx_std_14)
        target_compile_features(${LIB_NAME} PUBLIC c_std_11)
        target_compile_features(${LIB_NAME} PUBLIC cxx_std_14)
        set_c_compile_flags(${EXE_NAME})
        set_c_compile_flags(${LIB_NAME})

        # NOTE (Matteo): Since the app library is meant to be hot-reloaded, it is 
        # not linked explicitly; the dependency is still required to ensure proper
        # rebuild on changes
        add_dependencies(${EXE_NAME} ${LIB_NAME})

        # set_target_properties(${EXE_NAME} ${LIB_NAME} PROPERTIES UNITY_BUILD TRUE)
    endif()    
endforeach()

#---------#
# Testing #
#---------#

set(CLI_ENTRY ${CODE_DIR}/win32_cli.c)
set_source_files_properties(${CLI_ENTRY} PROPERTIES LANGUAGE C)

include(CTest)
enable_testing()

add_executable(test_buffer ${TESTS_DIR}/test_buffer.c ${CLI_ENTRY})
target_link_libraries(test_buffer PRIVATE foundation)
target_include_directories(test_buffer PRIVATE ${LIBS_DIR})
set_c_compile_flags(test_buffer)
add_test(test_buffer test_buffer)

add_executable(test_arena ${TESTS_DIR}/test_arena.c ${CLI_ENTRY})
target_link_libraries(test_arena PRIVATE foundation)
target_include_directories(test_arena PRIVATE ${LIBS_DIR})
set_c_compile_flags(test_arena)
add_test(test_arena test_arena)

add_executable(dummy ${TESTS_DIR}/dummy.c ${CLI_ENTRY})
target_link_libraries(dummy PRIVATE foundation)
target_include_directories(dummy PRIVATE ${LIBS_DIR})
set_c_compile_flags(dummy)
add_test(dummy dummy)

file(GLOB_RECURSE TEST_SOURCES ${TESTS_DIR}/threading/*.c ${CLI_ENTRY})
add_executable(test_threading ${TEST_SOURCES})
target_link_libraries(test_threading PRIVATE foundation)
target_include_directories(test_threading PRIVATE ${LIBS_DIR})
set_c_compile_flags(test_threading)
add_test(threading_basic test_threading)
add_test(threading_benaphore test_threading 0)
add_test(threading_auto_reset_event test_threading 1)
add_test(threading_mpmc_queue test_threading 2)

add_executable(test_files ${TESTS_DIR}/test_files.c ${CLI_ENTRY})
target_link_libraries(test_files PRIVATE foundation)
target_include_directories(test_files PRIVATE ${LIBS_DIR})
set_c_compile_flags(test_files)
add_test(test_files test_files)

add_executable(test_sort ${TESTS_DIR}/sort.c ${CLI_ENTRY})
target_link_libraries(test_sort PRIVATE foundation)
target_include_directories(test_sort PRIVATE ${LIBS_DIR})
set_c_compile_flags(test_sort)
add_test(test_sort test_sort)

# WINDOWS SPECIFIC

add_executable(test_odbc ${TESTS_DIR}/test_odbc.c ${CLI_ENTRY})
target_link_libraries(test_odbc PRIVATE foundation)
target_include_directories(test_odbc PRIVATE ${LIBS_DIR})
set_c_compile_flags(test_odbc)

add_executable(test_sockets ${TESTS_DIR}/test_sockets.c ${CLI_ENTRY})
target_link_libraries(test_sockets PRIVATE foundation)
target_include_directories(test_sockets PRIVATE ${LIBS_DIR})
set_c_compile_flags(test_sockets)

add_executable(test_iocp ${TESTS_DIR}/test_iocp.c ${CLI_ENTRY})
target_link_libraries(test_iocp PRIVATE foundation)
target_include_directories(test_iocp PRIVATE ${LIBS_DIR})
set_c_compile_flags(test_iocp)
