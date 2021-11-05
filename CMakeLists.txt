cmake_minimum_required(VERSION 3.20.0)

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

#---------------#
# Local scripts #
#---------------#

include(cmake/CompileFlags.cmake)

#------------------#
# Dependency paths #
#------------------#

set(EXT_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/ext)
set(CODE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/code)
set(TESTS_DIR ${CODE_DIR}/tests)

#----------------------#
# Dependency libraries #
#----------------------#

add_subdirectory(${CODE_DIR}/foundation)
add_subdirectory(${CODE_DIR}/gui)

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

set(APP_LIST
    demo 
    image_viewer
    learn_opengl)

foreach(APP ${APP_LIST})  
    message(STATUS "Configuring ${APP}")

    set(EXE_NAME ${APP})
    # NOTE (Matteo): _lib suffix added to prevent conflicts with .pdb files
    set(LIB_NAME ${APP}_lib) 

    set(APP_SOURCES)
    file(GLOB_RECURSE APP_SOURCES ${CODE_DIR}/${APP}/*.c)
    list(APPEND APP_SOURCES ${CODE_DIR}/gl/gload.c)
    set_source_files_properties(${APP_SOURCES} PROPERTIES LANGUAGE C)

    set(APP_RESOURCES)
    file(GLOB_RECURSE APP_RESOURCES ${CODE_DIR}/${APP}/*.rc)
    set_source_files_properties(${APP_RESOURCES} PROPERTIES LANGUAGE RC)

    add_executable(${EXE_NAME} WIN32 ${PLATFORM_SOURCES} ${APP_RESOURCES})
    add_library(${LIB_NAME} SHARED ${APP_SOURCES})

    target_compile_features(${EXE_NAME} PUBLIC c_std_11)
    target_compile_features(${LIB_NAME} PUBLIC c_std_11)

    target_include_directories(${EXE_NAME} PUBLIC ${CODE_DIR})
    target_include_directories(${LIB_NAME} PUBLIC ${CODE_DIR} ${EXT_DIR}/stb)

    target_link_libraries(${EXE_NAME} PRIVATE foundation gui_backend)
    target_link_libraries(${LIB_NAME} PRIVATE foundation gui ${OPENGL_gl_LIBRARY}) 

    set_c_compile_flags(${EXE_NAME})
    set_c_compile_flags(${LIB_NAME})

    set_target_properties(${EXE_NAME} ${LIB_NAME} PROPERTIES UNITY_BUILD TRUE)
endforeach()

#-------------------------#
# Vulkan test application #
#-------------------------#

set(APP vulkan_test)
message(STATUS "Configuring ${APP}")

set(EXE_NAME ${APP})
set(IMGUI_DIR ${EXT_DIR}/imgui)
set(BACKEND_DIR ${IMGUI_DIR}/backends)

find_package(Vulkan REQUIRED)

set(APP_SOURCES)
file(GLOB_RECURSE APP_SOURCES ${CODE_DIR}/${APP}/*.c)
set_source_files_properties(${APP_SOURCES} PROPERTIES LANGUAGE C)

set(BACKEND_SOURCES ${BACKEND_DIR}/imgui_impl_vulkan.cpp)

set(APP_RESOURCES)
file(GLOB_RECURSE APP_RESOURCES ${CODE_DIR}/${APP}/*.rc)
set_source_files_properties(${APP_RESOURCES} PROPERTIES LANGUAGE RC)

add_executable(${EXE_NAME} WIN32 ${APP_SOURCES} ${BACKEND_SOURCES} ${APP_RESOURCES})

target_compile_features(${EXE_NAME} PUBLIC c_std_11)
target_include_directories(${EXE_NAME} PUBLIC ${CODE_DIR} ${Vulkan_INCLUDE_DIRS} ${IMGUI_DIR})
target_link_libraries(${EXE_NAME} PRIVATE foundation gui_backend ${Vulkan_LIBRARIES})

set_c_compile_flags(${EXE_NAME})
set_target_properties(${EXE_NAME} PROPERTIES UNITY_BUILD TRUE)

execute_process(
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} -fshader-stage=vert -mfmt=num "${CODE_DIR}/${APP}/vert.glsl" -o "${CODE_DIR}/${APP}/vert.spv"
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} -fshader-stage=frag -mfmt=num "${CODE_DIR}/${APP}/frag.glsl" -o "${CODE_DIR}/${APP}/frag.spv"
    COMMAND_ERROR_IS_FATAL ANY)

#---------#
# Testing #
#---------#

include(CTest)
enable_testing()

add_executable(test_array ${TESTS_DIR}/test_array.c ${CONSOLE_ENTRY})
target_link_libraries(test_array PRIVATE foundation)
target_include_directories(test_array PRIVATE ${CODE_DIR})
set_c_compile_flags(test_array)
add_test(test_array test_array)

add_executable(test_arena ${TESTS_DIR}/test_arena.c ${CONSOLE_ENTRY})
target_link_libraries(test_arena PRIVATE foundation)
target_include_directories(test_arena PRIVATE ${CODE_DIR})
set_c_compile_flags(test_arena)
add_test(test_arena test_arena)

add_executable(dummy ${TESTS_DIR}/dummy.c ${CONSOLE_ENTRY})
target_link_libraries(dummy PRIVATE foundation)
target_include_directories(dummy PRIVATE ${CODE_DIR})
set_c_compile_flags(dummy)

add_executable(test_odbc ${TESTS_DIR}/test_odbc.c ${CONSOLE_ENTRY})
target_link_libraries(test_odbc PRIVATE foundation)
target_include_directories(test_odbc PRIVATE ${CODE_DIR})
set_c_compile_flags(test_odbc)

add_executable(test_files ${TESTS_DIR}/test_files.c ${CONSOLE_ENTRY})
target_link_libraries(test_files PRIVATE foundation)
target_include_directories(test_files PRIVATE ${CODE_DIR})
set_c_compile_flags(test_files)

# add_executable(test_threading ${TESTS_DIR}/test_threading.c ${CONSOLE_ENTRY})
# target_link_libraries(test_threading PRIVATE foundation)
# target_include_directories(test_threading PRIVATE ${CODE_DIR})
# set_c_compile_flags(test_threading)

file(GLOB_RECURSE TEST_SOURCES ${TESTS_DIR}/threading/*.c ${CONSOLE_ENTRY})
add_executable(test_threading ${TEST_SOURCES})
target_link_libraries(test_threading PRIVATE foundation)
target_include_directories(test_threading PRIVATE ${CODE_DIR})
set_c_compile_flags(test_threading)
add_test(test_benaphore test_threading 0)
add_test(test_auto_reset_event test_threading 1)
add_test(test_mpmc_queue test_threading 2)
