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
add_subdirectory(${LIBS_DIR}/gui)

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

#----------------------#
# Common platform code #
#----------------------#

set(PLATFORM_SOURCES ${CODE_DIR}/platform.c)
set_source_files_properties(${PLATFORM_SOURCES} PROPERTIES LANGUAGE C)

set(CONSOLE_ENTRY ${CODE_DIR}/console.c)
set_source_files_properties(${CONSOLE_ENTRY} PROPERTIES LANGUAGE C)

#--------------#
# Applications #
#--------------#

file(GLOB APPS RELATIVE ${APPS_DIR} ${APPS_DIR}/*)
foreach(APP ${APPS})
    if(IS_DIRECTORY ${APPS_DIR}/${APP})
        add_subdirectory(${APPS_DIR}/${APP})
    endif()    
endforeach()

#---------#
# Testing #
#---------#

include(CTest)
enable_testing()

add_executable(test_buffer ${TESTS_DIR}/test_buffer.c ${CONSOLE_ENTRY})
target_link_libraries(test_buffer PRIVATE foundation)
target_include_directories(test_buffer PRIVATE ${LIBS_DIR})
set_c_compile_flags(test_buffer)
add_test(test_buffer test_buffer)

add_executable(test_arena ${TESTS_DIR}/test_arena.c ${CONSOLE_ENTRY})
target_link_libraries(test_arena PRIVATE foundation)
target_include_directories(test_arena PRIVATE ${LIBS_DIR})
set_c_compile_flags(test_arena)
add_test(test_arena test_arena)

add_executable(dummy ${TESTS_DIR}/dummy.c ${CONSOLE_ENTRY})
target_link_libraries(dummy PRIVATE foundation)
target_include_directories(dummy PRIVATE ${LIBS_DIR})
set_c_compile_flags(dummy)

# set (ASANFLAGS 
#     -fsanitize=address
#     -fno-common
#     -fno-omit-frame-pointer)
# target_link_libraries(dummy PRIVATE ${ASANFLAGS})

add_executable(test_odbc ${TESTS_DIR}/test_odbc.c ${CONSOLE_ENTRY})
target_link_libraries(test_odbc PRIVATE foundation)
target_include_directories(test_odbc PRIVATE ${LIBS_DIR})
set_c_compile_flags(test_odbc)

add_executable(test_files ${TESTS_DIR}/test_files.c ${CONSOLE_ENTRY})
target_link_libraries(test_files PRIVATE foundation)
target_include_directories(test_files PRIVATE ${LIBS_DIR})
set_c_compile_flags(test_files)

# add_executable(test_threading ${TESTS_DIR}/test_threading.c ${CONSOLE_ENTRY})
# target_link_libraries(test_threading PRIVATE foundation)
# target_include_directories(test_threading PRIVATE ${LIBS_DIR})
# set_c_compile_flags(test_threading)

file(GLOB_RECURSE TEST_SOURCES ${TESTS_DIR}/threading/*.c ${CONSOLE_ENTRY})
add_executable(test_threading ${TEST_SOURCES})
target_link_libraries(test_threading PRIVATE foundation)
target_include_directories(test_threading PRIVATE ${LIBS_DIR})
set_c_compile_flags(test_threading)
add_test(test_benaphore test_threading 0)
add_test(test_auto_reset_event test_threading 1)
add_test(test_mpmc_queue test_threading 2)
