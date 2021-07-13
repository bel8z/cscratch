#include "threading.h"

#ifdef CF_OS_WIN32
#    include "threading_win32.c"
#else
#    error "Threading API not implemented for this platform"
#endif
