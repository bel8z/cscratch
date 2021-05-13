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

set(DEMO_SOURCES 
    ${CODE_DIR}/platform.c
    ${CODE_DIR}/platform_win32.c
    ${CODE_DIR}/demo/app.c)
set_source_files_properties(${DEMO_SOURCES} PROPERTIES LANGUAGE C)

add_executable(demo ${DEMO_SOURCES})

target_compile_features(demo PUBLIC c_std_11)

target_include_directories(demo PUBLIC  ".")

target_link_libraries(demo PRIVATE
    foundation
    backend)

set_c_compile_flags(demo)

#--------------------------
# Image viewer application
#--------------------------

set(IMAGE_SOURCES 
    ${CODE_DIR}/platform.c
    ${CODE_DIR}/platform_win32.c
    ${CODE_DIR}/image/app.c 
    ${CODE_DIR}/image/image.c)
set_source_files_properties(${IMAGE_SOURCES} PROPERTIES LANGUAGE C)

add_executable(image_viewer ${IMAGE_SOURCES})

target_compile_features(image_viewer PUBLIC c_std_11)

target_include_directories(image_viewer PUBLIC 
                            ${CODE_DIR}
                            ${EXT_DIR}/stb)

target_link_libraries(image_viewer PRIVATE
    foundation
    backend)

set_c_compile_flags(image_viewer)

#-----------
# Testing
#-----------

include(CTest)
enable_testing()

add_executable(test_array tests/test_array.c)
target_link_libraries(test_array PRIVATE foundation)
target_include_directories(test_array PRIVATE ${CODE_DIR})
set_c_compile_flags(test_array)

add_test(test_array test_array)
