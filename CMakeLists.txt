cmake_minimum_required(VERSION 3.15.0)
project(scratch VERSION 0.1.0 LANGUAGES C)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

set(SOURCES sparse.c)
set_source_files_properties(${SOURCES} PROPERTIES LANGUAGE C)

add_executable( ${PROJECT_NAME} ${SOURCES})

# Link target with multi-threaded statically-linked runtime library with or without debug 
# information depending on the configuration.
set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include(cmake/CompileFlags.cmake)
set_c_compile_flags(${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC c_std_11)

add_subdirectory(foundation)

target_link_libraries(${PROJECT_NAME} foundation)
