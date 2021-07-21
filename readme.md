# Coding conventions

* Indent 4 spaces, no tabs
* Always use braces, except for single line statements
* Open braces always on a new line (i.e. "Allman" style)
* Types are _PascalCase_
* Functions are _camelCase_
* Variables and fields are _snake_case_ (function pointer fields may use function style)
* Globals are prefixed with g_ and should be used sparingly (the common case is in the file containing the platform layer, which acts as the entry point.)
* Macros are _ALL_CAPS_SNAKE_CASE_, except if they have function semantics (in which case they are typed as functions)
  
## A note about function naming:

Ideally, all functions should be namespaced using a prefix which states the system they belong to (e.g. _appSomething_ for top level application code, _guiButton_, _zipPack_, etc...) or the struct they operate on (which is quite the same concept).

For library public functions, this prefix should start with the library prefix (e.g. _igCreateContext_, _cfArrayPush_).

# Missing features

- [ ] Move OS basics to a library
- [ ] Clean up architecture (platform layer, app layer, foundation and OS libraries)
- [ ] Move memory tracking to foundation library, and let the OS provide only VM and heap functions
- [ ] Strings API
    - [ ] Better nomenclature
    - [ ] Provide utilities for both fixed and dynamic string buffers vs. including a fixed buffer in string builder
- [ ] Paths API
    - [ ] Same as strings API, to which it is dependent
- [ ] Atomics?
- [ ] Networking?
