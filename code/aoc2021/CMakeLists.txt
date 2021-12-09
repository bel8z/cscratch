set(APP aoc2021)
message(STATUS "Configuring ${APP}")

set(APP_SOURCES ${CODE_DIR}/${APP}/day1.c)
set_source_files_properties(${APP_SOURCES} PROPERTIES LANGUAGE C)

set(APP_RESOURCES)
file(GLOB_RECURSE APP_RESOURCES ${CODE_DIR}/${APP}/*.rc)
set_source_files_properties(${APP_RESOURCES} PROPERTIES LANGUAGE RC)

add_executable(${APP} ${APP_SOURCES} ${APP_RESOURCES} ${CONSOLE_ENTRY})

target_compile_features(${APP} PUBLIC c_std_11)
target_include_directories(${APP} PUBLIC ${CODE_DIR} ${Vulkan_INCLUDE_DIRS} ${IMGUI_DIR})
target_link_libraries(${APP} PRIVATE foundation gui_backend ${Vulkan_LIBRARIES})

set_c_compile_flags(${APP})
set_target_properties(${APP} PROPERTIES UNITY_BUILD TRUE)

if (MSVC)
    foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
        set(OUT_DIR "${CMAKE_BINARY_DIR}/${CONFIG_TYPE}")

        message(STATUS "Output directory (${CONFIG_TYPE}): ${OUT_DIR}")

        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_TYPE} "${OUT_DIR}")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_TYPE} "${OUT_DIR}")
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE} "${OUT_DIR}")

        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/input DESTINATION "${OUT_DIR}")
    endforeach()
else()
    set(OUT_DIR "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}")

    message(STATUS "Output directory: ${OUT_DIR}")

    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUT_DIR}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUT_DIR}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUT_DIR}")
    
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/input DESTINATION "${OUT_DIR}")
endif()
