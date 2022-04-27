message(STATUS "Configuring ${APP}")

set(EXE_NAME ${APP})
# NOTE (Matteo): _lib suffix added to prevent conflicts with .pdb files
set(LIB_NAME ${APP}_lib) 

set(APP_SOURCES)
file(GLOB_RECURSE APP_SOURCES ${APPS_DIR}/${APP}/*.c)
list(APPEND APP_SOURCES ${LIBS_DIR}/gl/gload.c)
set_source_files_properties(${APP_SOURCES} PROPERTIES LANGUAGE C)

set(APP_RESOURCES)
file(GLOB_RECURSE APP_RESOURCES ${APPS_DIR}/${APP}/*.rc)
set_source_files_properties(${APP_RESOURCES} PROPERTIES LANGUAGE RC)

add_executable(${EXE_NAME} WIN32 ${PLATFORM_SOURCES} ${APP_RESOURCES})
add_library(${LIB_NAME} SHARED ${APP_SOURCES})

target_compile_features(${EXE_NAME} PUBLIC c_std_11)
target_compile_features(${LIB_NAME} PUBLIC c_std_11)

target_include_directories(${EXE_NAME} PUBLIC ${LIBS_DIR})
target_include_directories(${LIB_NAME} PUBLIC ${LIBS_DIR} ${EXT_DIR}/stb)

target_link_libraries(${EXE_NAME} PRIVATE foundation gui_gl3)
target_link_libraries(${LIB_NAME} PRIVATE foundation gui ${OPENGL_gl_LIBRARY}) 

target_link_libraries(${EXE_NAME} PUBLIC Imm32)
target_link_libraries(${LIB_NAME} PUBLIC Imm32)

set_c_compile_flags(${EXE_NAME})
set_c_compile_flags(${LIB_NAME})

set_target_properties(${EXE_NAME} ${LIB_NAME} PROPERTIES UNITY_BUILD TRUE)
