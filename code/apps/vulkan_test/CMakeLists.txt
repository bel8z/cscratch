message(STATUS "Configuring ${APP}")

set(EXE_NAME ${APP})
set(IMGUI_DIR ${EXT_DIR}/imgui)

find_package(Vulkan REQUIRED)
message(STATUS "Vulkan_INCLUDE_DIRS: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "Vulkan_LIBRARIES: ${Vulkan_LIBRARIES}")

set(APP_SOURCES)
file(GLOB_RECURSE APP_SOURCES 
    ${APPS_DIR}/${APP}/*.c
    ${APPS_DIR}/${APP}/*.cpp)

set(APP_RESOURCES)
file(GLOB_RECURSE APP_RESOURCES ${APPS_DIR}/${APP}/*.rc)
set_source_files_properties(${APP_RESOURCES} PROPERTIES LANGUAGE RC)

add_executable(${EXE_NAME} WIN32 ${APP_SOURCES} ${APP_RESOURCES})

target_compile_features(${EXE_NAME} PUBLIC c_std_11)
target_include_directories(${EXE_NAME} PUBLIC ${CODE_DIR} ${Vulkan_INCLUDE_DIRS} ${IMGUI_DIR} PRIVATE ${IMGUI_DIR}/backends)
target_link_libraries(${EXE_NAME} PRIVATE foundation gui_win ${Vulkan_LIBRARIES})

set_c_compile_flags(${EXE_NAME})
set_target_properties(${EXE_NAME} PROPERTIES UNITY_BUILD TRUE)

execute_process(
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} -fshader-stage=vert -mfmt=num "${APPS_DIR}/${APP}/vert.glsl" -o "${APPS_DIR}/${APP}/vert.spv"
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} -fshader-stage=frag -mfmt=num "${APPS_DIR}/${APP}/frag.glsl" -o "${APPS_DIR}/${APP}/frag.spv"
    COMMAND_ERROR_IS_FATAL ANY)
