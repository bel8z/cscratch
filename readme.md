Coding conventions
--- 

* Indent 4 spaces, no tabs
* Always use braces, except for single line statements
* Open braces always on a new line (i.e. "Allman" style)
* Types are PascalCase
* Functions are camelCase
* Variables and fields are snake_case (function pointer fields may use function style)
* Globals are prefixed with g_ and should be used sparingly (the common case is in the file containing the platform layer, which acts as the entry point.)
* Macros are ALL_CAPS_SNAKE_CASE, except if they have function semantics (in which case they are typed as functions)
  
A note about function naming:
---
Ideally, all functions should be namespaced using a prefix which states the system they belong to (e.g. appSomething for top level application code, guiButton, zipPack, etc...) or the struct they operate on (which is quite the same concept).

For library public functions, this prefix should start with the library prefix (e.g. igCreateContext, cfArrayPush).
