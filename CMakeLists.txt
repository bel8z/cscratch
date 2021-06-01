cmake_minimum_required(VERSION 3.20.0)

# Force linking against MSVC runtime static library for all targets
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(scratch VERSION 0.1.0)

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

#-------------------
# Dependency paths
#-------------------

set(EXT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ext)
set(CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/code)

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

add_subdirectory(${CODE_DIR}/foundation)
add_subdirectory(${CODE_DIR}/backend)

#--------------------------
# Demo application
#--------------------------
set(EXE_NAME demo)

message(STATUS "Configuring ${EXE_NAME}")

set(DEMO_SOURCES 
    ${CODE_DIR}/platform.c
    ${CODE_DIR}/win32/win32_platform.c
    ${CODE_DIR}/gui.c
    ${CODE_DIR}/demo/app.c)
set_source_files_properties(${DEMO_SOURCES} PROPERTIES LANGUAGE C)

add_executable(${EXE_NAME} WIN32 ${DEMO_SOURCES})

target_compile_features(${EXE_NAME} PUBLIC c_std_11)

target_include_directories(${EXE_NAME} PUBLIC  ".")

target_link_libraries(${EXE_NAME} PRIVATE
    foundation
    backend)

set_c_compile_flags(${EXE_NAME})

#--------------------------
# Image viewer application
#--------------------------
set(EXE_NAME image_viewer)

message(STATUS "Configuring ${EXE_NAME}")

set(IMAGE_SOURCES 
    ${CODE_DIR}/platform.c
    ${CODE_DIR}/win32/win32_platform.c
    ${CODE_DIR}/gui.c
    ${CODE_DIR}/image/app.c 
    ${CODE_DIR}/image/image.c)
set_source_files_properties(${IMAGE_SOURCES} PROPERTIES LANGUAGE C)

set(IMAGE_RESOURCES 
    ${CODE_DIR}/image/version.rc)
set_source_files_properties(${IMAGE_RESOURCES} PROPERTIES LANGUAGE RC)

add_executable(${EXE_NAME} WIN32 ${IMAGE_SOURCES} ${IMAGE_RESOURCES})

target_compile_features(${EXE_NAME} PUBLIC c_std_11)

target_include_directories(${EXE_NAME} PUBLIC 
                            ${CODE_DIR}
                            ${EXT_DIR}/stb)

target_link_libraries(${EXE_NAME} PRIVATE
    foundation
    backend)

set_c_compile_flags(${EXE_NAME})

#-----------
# Testing
#-----------

include(CTest)
enable_testing()

add_executable(test_array tests/test_array.c)
target_link_libraries(test_array PRIVATE foundation)
target_include_directories(test_array PRIVATE ${CODE_DIR})
set_c_compile_flags(test_array)

add_executable(test_win32_threads tests/test_win32_threads.c)
target_link_libraries(test_win32_threads PRIVATE foundation)
target_include_directories(test_win32_threads PRIVATE ${CODE_DIR})
set_c_compile_flags(test_win32_threads)

add_test(test_array test_array)
add_test(test_win32_threads test_win32_threads)
