#include "win32_platform.c"

/// Cross-platform entry point for console applications
extern I32 consoleMain(Platform *platform, CommandLine *cmd_line);

I32
main(I32 argc, Cstr argv[])
{
    win32PlatformInit();
    I32 result = consoleMain(&g_platform, &(CommandLine){.arg = argv, .len = (Usize)argc});
    win32PlatformShutdown();
    return result;
}
