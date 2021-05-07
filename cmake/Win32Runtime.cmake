
function(set_win32_runtime_static project_name)
    # Link target with multi-threaded statically-linked runtime library with or without debug 
    # information depending on the configuration.
    set_property(TARGET ${project_name} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endfunction()
