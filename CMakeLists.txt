cmake_minimum_required(VERSION 3.15.0)
project(scratch VERSION 0.1.0 LANGUAGES C)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

set(SOURCES tests/test_array.c)
set_source_files_properties(${SOURCES} PROPERTIES LANGUAGE C)

add_executable( ${PROJECT_NAME} ${SOURCES})

include(cmake/CompileFlags.cmake)
set_c_compile_flags(${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC c_std_11)

add_subdirectory(foundation)

target_link_libraries(${PROJECT_NAME} foundation)
target_include_directories(${PROJECT_NAME} PUBLIC ".")
