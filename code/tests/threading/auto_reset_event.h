#include "foundation/threading.h"

typedef struct CfAutoResetEvent
{
    CfSemaphore sema;
    AtomI32 status;
} CfAutoResetEvent;

CF_API void cfArEventInit(CfAutoResetEvent *event);
CF_API void cfArEventWait(CfAutoResetEvent *event);
CF_API void cfArEventSignal(CfAutoResetEvent *event);
