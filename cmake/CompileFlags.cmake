function(set_c_compile_flags project_name)
    set(PROJECT_WARNINGS)

    option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors" TRUE)
    option(CHECK_PADDING "Check extra padding in structs" FALSE)

    set(MSVC_FLAGS
        /W4 # Baseline reasonable warnings
        /w14242 # 'identifier': conversion from 'type1' to 'type1', possible loss of data
        /w14254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
        /w14287 # 'operator': unsigned/negative constant mismatch
        /w14263 # 'function': member function does not override any base class virtual member function
        /w14265 # 'classname': class has virtual functions, but destructor is not virtual instances of this class may not
                # be destructed correctly
        /we4289 # nonstandard extension used: 'variable': loop control variable declared in the for-loop is used outside
                # the for-loop scope
        /w14296 # 'operator': expression is always 'boolean_value'
        /w14311 # 'variable': pointer truncation from 'type1' to 'type2'
        /w14545 # expression before comma evaluates to a function which is missing an argument list
        /w14546 # function call before comma missing argument list
        /w14547 # 'operator': operator before comma has no effect; expected operator with side-effect
        /w14549 # 'operator': operator before comma has no effect; did you intend 'operator'?
        /w14555 # expression has no effect; expected expression with side- effect
        /w14619 # pragma warning: there is no warning number 'number'
        /w14640 # Enable warning on thread un-safe static member initialization
        /w14826 # Conversion from 'type1' to 'type_2' is sign-extended. This may cause unexpected runtime behavior.
        /w14905 # wide string literal cast to 'LPSTR'
        /w14906 # string literal cast to 'LPWSTR'
        /w14928 # illegal copy-initialization; more than one user-defined conversion has been implicitly applied
        /permissive- # standards conformance mode for MSVC compiler.

        /volatile:iso #standard conformant volatile specifier

        # Disabled
        /wd4201 #nonstandard extension used: nameless struct/union 
    )

    set(CLANG_WARNINGS
        -Wall
        -Wextra # reasonable and standard
        -Wpedantic # warn if non-standard C is used       
        -Wshadow # warn the user if a variable declaration shadows one from a parent context
        -Wunused # warn on anything being unused
        -Wnull-dereference # warn if a null dereference is detected
        -Wconversion # warn on type conversions that may lose data
        -Wsign-conversion # warn on sign conversions
        -Wdouble-promotion # warn if float is implicit promoted to double
        -Wformat=2 # warn on security issues around functions that format output (ie printf)
        -Wnon-virtual-dtor # warn the user if a class with virtual functions has a non-virtual destructor. 
                           # This helps catch hard to track down memory errors
        -Woverloaded-virtual # warn if you overload (not override) a virtual function

        #-Wcast-align # warn for potential performance problem casts
        
        # Allow anonymous struct members, they're useful
        -Wno-nested-anon-types

        # Allow variadic macros to be called without an argument in place of the ellipsis
        -Wno-gnu-zero-variadic-macro-arguments

        # Converting int to float by default seems
        -Wno-implicit-int-float-conversion

        # Disable C++ exceptions and RTTI
        -fno-exceptions
        -fno-rtti
    )

    if (WARNINGS_AS_ERRORS)
        set(CLANG_FLAGS ${CLANG_FLAGS} -Werror)
        set(MSVC_FLAGS ${MSVC_FLAGS} /WX)
    endif()

    if (CHECK_PADDING)
        set(CLANG_FLAGS ${CLANG_FLAGS} -Wpadded)
        set(MSVC_FLAGS ${MSVC_FLAGS} /we4820 /we4121)
    endif()

    set(GCC_FLAGS
        ${CLANG_FLAGS}
        -Wmisleading-indentation # warn if indentation implies blocks where blocks do not exist
        -Wduplicated-cond # warn if if / else chain has duplicated conditions
        -Wduplicated-branches # warn if if / else branches have duplicated code
        -Wlogical-op # warn about logical operations being used where bitwise were probably wanted
        -Wuseless-cast # warn if you perform a cast to the same type
    )

    if(MSVC)
        set(PROJECT_FLAGS ${MSVC_FLAGS})
    elseif(CMAKE_C_COMPILER_ID MATCHES ".*Clang")
        # Enable UBSAN for debug builds
        add_compile_options("$<$<CONFIG:DEBUG>:-fsanitize=undefined>")
        add_compile_options("$<$<CONFIG:DEBUG>:-fno-omit-frame-pointer>")
        add_compile_options(-fcolor-diagnostics)
        set(PROJECT_FLAGS ${CLANG_FLAGS})
    elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-fdiagnostics-color=always)
        set(PROJECT_FLAGS ${GCC_FLAGS})
    else()
        message(AUTHOR_WARNING "No compiler flags set for '${CMAKE_C_COMPILER_ID}' compiler.")
    endif()

  target_compile_options(${project_name} INTERFACE ${PROJECT_FLAGS})
endfunction()