cmake_minimum_required(VERSION 3.20.0)

#----------------
# Global options
#----------------

# Force linking against MSVC runtime static library for all targets
cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

#---------------------
# Project declaration
#---------------------

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
set(TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests)

#-----------------------
# Dependency libraries
#-----------------------

add_subdirectory(${CODE_DIR}/foundation)
add_subdirectory(${CODE_DIR}/gui)

#-----------------------
# Common resource files
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

#--------------------------
# Common platform code
#--------------------------

set(PLATFORM_SOURCES 
    ${CODE_DIR}/platform.c
    ${CODE_DIR}/win32/win32_platform.c
    ${CODE_DIR}/win32/win32_threading.c)

set_source_files_properties(${PLATFORM_SOURCES} PROPERTIES LANGUAGE C)

#--------------------------
# Applications
#--------------------------
set(APP_LIST
    demo 
    image_viewer)

foreach(APP ${APP_LIST})  
    message(STATUS "Configuring ${APP}")

    set(EXE_NAME ${APP})
    set(DLL_NAME ${APP}_dll)

    set(APP_SOURCES)
    set(APP_RESOURCES)

    file(GLOB_RECURSE APP_SOURCES ${CODE_DIR}/${APP}/*.c)
    file(GLOB_RECURSE APP_RESOURCES ${CODE_DIR}/${APP}/*.rc)

    list(APPEND APP_SOURCES
            ${CODE_DIR}/gl/gload.c)

    set_source_files_properties(${APP_SOURCES} PROPERTIES LANGUAGE C)
    set_source_files_properties(${APP_RESOURCES} PROPERTIES LANGUAGE RC)

    add_executable(${EXE_NAME} WIN32 ${PLATFORM_SOURCES} ${APP_RESOURCES})
    add_library(${DLL_NAME} SHARED ${APP_SOURCES})

    target_compile_features(${EXE_NAME} PUBLIC c_std_11)
    target_compile_features(${DLL_NAME} PUBLIC c_std_11)

    target_include_directories(${EXE_NAME} PUBLIC ${CODE_DIR})
    target_include_directories(${DLL_NAME} PUBLIC ${CODE_DIR} ${EXT_DIR}/stb)

    target_link_libraries(${EXE_NAME} PRIVATE foundation gui_backend)
    target_link_libraries(${DLL_NAME} PRIVATE foundation gui ${OPENGL_gl_LIBRARY}) 

    set_c_compile_flags(${EXE_NAME})
    set_c_compile_flags(${DLL_NAME})

    set_target_properties(${DLL_NAME} PROPERTIES OUTPUT_NAME ${EXE_NAME})
endforeach()

#-----------
# Testing
#-----------

include(CTest)
enable_testing()

add_executable(test_array ${TESTS_DIR}/test_array.c)
target_link_libraries(test_array PRIVATE foundation)
target_include_directories(test_array PRIVATE ${CODE_DIR})
set_c_compile_flags(test_array)

add_executable(test_win32_threads 
    ${CODE_DIR}/win32/win32_threading.c
    ${TESTS_DIR}/test_win32_threads.c)
target_link_libraries(test_win32_threads PRIVATE foundation)
target_include_directories(test_win32_threads PRIVATE ${CODE_DIR})
set_c_compile_flags(test_win32_threads)

add_test(test_array test_array)
add_test(test_win32_threads test_win32_threads)
