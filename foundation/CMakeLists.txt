cmake_minimum_required(VERSION 3.15.0)

file(GLOB_RECURSE LIB_SOURCES  "*.c")
add_library(foundation STATIC ${LIB_SOURCES})

get_directory_property(PARENT_DIR PARENT_DIRECTORY)
message(STATUS "foundation parent directory: ${PARENT_DIR}")

include(${PARENT_DIR}/cmake/CompileFlags.cmake)

# target_include_directories(foundation INTERFACE ".")
target_compile_features(foundation PUBLIC c_std_11) 

set_c_compile_flags(foundation)
