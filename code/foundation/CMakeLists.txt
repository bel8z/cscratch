cmake_minimum_required(VERSION 3.15.0)

set(
    LIB_SOURCES  
        "memory.c"
        "list.c"
        "paths.c"
        "strings.c"
        "threading.c"
)
add_library(foundation STATIC ${LIB_SOURCES})

message(STATUS "foundation sources: ${LIB_SOURCES}")

get_directory_property(PARENT_DIR PARENT_DIRECTORY)
message(STATUS "foundation parent directory: ${PARENT_DIR}")

include(${PARENT_DIR}/cmake/CompileFlags.cmake)

# target_include_directories(foundation INTERFACE ".")
target_compile_features(foundation PUBLIC c_std_11) 

set_c_compile_flags(foundation)
