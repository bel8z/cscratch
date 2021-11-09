function(configure_asan_profile)
    get_property(multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

    if(multi_config)
        if(NOT "Asan" IN_LIST CMAKE_CONFIGURATION_TYPES)
            list(APPEND CMAKE_CONFIGURATION_TYPES Asan)
        endif()
    else()
        set(build_types Asan Debug Release RelWithDebInfo MinSizeRel)
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${build_types}")

        if(CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE IN_LIST build_types)
            message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE}")
        endif()
    endif()

    set(CMAKE_C_FLAGS_ASAN
            "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer" CACHE STRING
            "Flags used by the C compiler for Asan build type or configuration." FORCE)

    set(CMAKE_CXX_FLAGS_ASAN
            "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer" CACHE STRING
            "Flags used by the C++ compiler for Asan build type or configuration." FORCE)

    set(CMAKE_EXE_LINKER_FLAGS_ASAN
            "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=address" CACHE STRING
            "Linker flags to be used to create executables for Asan build type." FORCE)

    set(CMAKE_SHARED_LINKER_FLAGS_ASAN
            "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=address" CACHE STRING
            "Linker lags to be used to create shared libraries for Asan build type." FORCE)
endfunction()
