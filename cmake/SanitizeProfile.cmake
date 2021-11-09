function(configure_sanitize_profile)
    get_property(multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

    if(multi_config)
        if(NOT "Sanitize" IN_LIST CMAKE_CONFIGURATION_TYPES)
            list(APPEND CMAKE_CONFIGURATION_TYPES Sanitize)
        endif()
    else()
        set(build_types Sanitize Debug Release RelWithDebInfo MinSizeRel)
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${build_types}")

        if(CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE IN_LIST build_types)
            message(FATAL_ERROR "Invalid build type: ${CMAKE_BUILD_TYPE}")
        endif()
    endif()

    # NOTE (MATTEO): Leak sanitizer not supported on windows
    # set(SANITIZERS "address,undefined,leak") 
    set(SANITIZERS "address,undefined") 

    set(CMAKE_C_FLAGS_SANITIZE
            "${CMAKE_C_FLAGS_DEBUG} -fsanitize=${SANITIZERS} -fno-omit-frame-pointer" CACHE STRING
            "Flags used by the C compiler for Sanitize build type or configuration." FORCE)

    set(CMAKE_CXX_FLAGS_SANITIZE
            "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=${SANITIZERS} -fno-omit-frame-pointer" CACHE STRING
            "Flags used by the C++ compiler for Sanitize build type or configuration." FORCE)

    set(CMAKE_EXE_LINKER_FLAGS_SANITIZE
            "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=${SANITIZERS}" CACHE STRING
            "Linker flags to be used to create executables for Sanitize build type." FORCE)

    set(CMAKE_SHARED_LINKER_FLAGS_SANITIZE
            "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} -fsanitize=${SANITIZERS}" CACHE STRING
            "Linker lags to be used to create shared libraries for Sanitize build type." FORCE)
endfunction()
