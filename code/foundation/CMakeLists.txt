cmake_minimum_required(VERSION 3.15.0)

set(LIB_SOURCES  
    "colors.c"
    "list.c"
    "log.c"
    "fs.c"
    "memory.c"
    "paths.c"
    "strings.c"
    "task.c"
    "threading.c")

set_source_files_properties(${LIB_SOURCES} PROPERTIES LANGUAGE C)

add_library(foundation     STATIC ${LIB_SOURCES})
add_library(foundation_dll SHARED ${LIB_SOURCES})

message(STATUS "foundation sources: ${LIB_SOURCES}")

get_directory_property(PARENT_DIR PARENT_DIRECTORY)
message(STATUS "foundation parent directory: ${PARENT_DIR}")

include(${PARENT_DIR}/cmake/CompileFlags.cmake)

target_compile_features(foundation     PUBLIC c_std_11) 
target_compile_features(foundation_dll PUBLIC c_std_11) 

set_c_compile_flags(foundation)
set_c_compile_flags(foundation_dll)

set_target_properties(foundation foundation_dll PROPERTIES UNITY_BUILD TRUE)
