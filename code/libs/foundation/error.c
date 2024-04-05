#include "error.h"

// TODO (Matteo): Get rid of it?
// At the moment it is required for printing assertion failures to stderr
#include <stdio.h>

static ErrorLogFn *g_handler = NULL;
static ErrorLogFn *g_context = NULL;

void
errorLog(Cstr format, ...)
{
    va_list args;

    va_start(args, format);

    if (g_handler)
    {
        g_handler(format, args, g_context);
    }
    else
    {
        vfprintf(stderr, format, args);
    }
}

void
errorInstallHandler(ErrorLogFn *handler, void *context)
{
    // TODO (Matteo): Protect with semaphore?
    CF_ASSERT(g_handler == NULL || handler == NULL, "Handler already installed");
    g_handler = handler;
    g_context = context;
}