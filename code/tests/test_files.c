#include "foundation/core.h"
#include "foundation/paths.h"
#include "foundation/win32.h"

#include <stdio.h>

typedef struct DirIterator
{
    U8 opaque[1024];
} DirIterator;

typedef struct Win32DirIterator
{
    HANDLE finder;
    Char8 buffer[sizeof(DirIterator) - sizeof(HANDLE)];
} Win32DirIterator;

CF_STATIC_ASSERT(sizeof(DirIterator) >= sizeof(Win32DirIterator),
                 "DirIterator storage size is too small");

bool win32DirIterStart(DirIterator *self, Str dir_path);
bool win32DirIterNext(DirIterator *self, Str *filename);
void win32DirIterEnd(DirIterator *self);

bool
win32DirIterStart(DirIterator *self, Str dir_path)
{
    CF_ASSERT_NOT_NULL(self);

    Win32DirIterator *iter = (Win32DirIterator *)self->opaque;
    WIN32_FIND_DATAW data = {0};
    Char16 buffer[1024];

    // Encode path to UTF16
    I32 size = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED,         //
                                   dir_path.buf, (I32)dir_path.len, //
                                   buffer, (I32)CF_ARRAY_SIZE(buffer));

    if (size < 0 || (Usize)size >= CF_ARRAY_SIZE(buffer) - 2)
    {
        CF_ASSERT(false, "Encoding error or overflow");
        return false;
    }

    // Append a wildcard (D:)
    buffer[size] = L'*';
    buffer[size + 1] = 0;

    // Start iteration
    iter->finder = FindFirstFileW(buffer, &data);
    if (iter->finder == INVALID_HANDLE_VALUE) return false;

    // Skip to ".." so that the "advance" method can call FindNextFileW directly
    if (!wcscmp(data.cFileName, L".") && !FindNextFileW(iter->finder, &data))
    {
        FindClose(iter->finder);
        iter->finder = INVALID_HANDLE_VALUE;
        return false;
    }

    return true;
}

bool
win32DirIterNext(DirIterator *self, Str *filename)
{
    CF_ASSERT_NOT_NULL(self);

    Win32DirIterator *iter = (Win32DirIterator *)self->opaque;
    WIN32_FIND_DATAW data = {0};

    if (!FindNextFileW(iter->finder, &data)) return false;

    I32 size = WideCharToMultiByte(CP_UTF8, 0, data.cFileName, -1, iter->buffer,
                                   CF_ARRAY_SIZE(iter->buffer), 0, false);

    // NOTE (Matteo): Truncation is considered an error
    // TODO (Matteo): Maybe require a bigger buffer?
    if (size < 0 || size == CF_ARRAY_SIZE(iter->buffer)) return false;

    CF_ASSERT(size > 0, "Which filename can have a size of 0???");

    filename->buf = iter->buffer;
    filename->len = (Usize)size;

    return true;
}

void
win32DirIterEnd(DirIterator *self)
{
    CF_ASSERT_NOT_NULL(self);

    Win32DirIterator *iter = (Win32DirIterator *)self->opaque;
    FindClose(iter->finder);
}

void
pathPrint(Str p)
{
    if (strValid(p)) printf("%.*s\n", (I32)p.len, p.buf);
}

int
main(void)
{
    printf("Splitting paths:\n");

    Str p = {0};

    p = pathSplitName(strFromCstr("c:\\Temp/IOT\\2021_02_25/"));
    pathPrint(p);

    p = pathSplitName(
        strFromCstr("//srvfile/AreaComune/Utenti/Matteo.Belotti/Lavori/Generatore/!Src/"
                    "EyeGoal-COREStation.zip"));
    pathPrint(p);

    p = pathSplitName(strFromCstr("Gianni Morandi"));
    pathPrint(p);

    Str dirname = strFromCstr("c:\\Temp/IOT\\2021_02_25/");
    Str filename = strFromCstr("//srvfile/AreaComune/Utenti/Matteo.Belotti/Lavori/Generatore/!Src/"
                               "EyeGoal-COREStation.zip");

    PathSplitIter pi = {0};
    pathSplitStart(&pi, dirname);

    while (pathSplitNext(&pi))
    {
        pathPrint(pi.curr);
    }

    pathSplitStart(&pi, filename);

    while (pathSplitNext(&pi))
    {
        pathPrint(pi.curr);
    }

    printf("Browsing dir:\n");

    // Platform plat = cfPlatformCreate();
    DirIterator iter = {0};

    if (win32DirIterStart(&iter, dirname))
    {
        Str f = {0};
        while (win32DirIterNext(&iter, &f))
        {
            pathPrint(f);
        }

        win32DirIterEnd(&iter);
    }

    // U32 sz = 0;
    // char *f = plat.fs.fileOpenDialog(NULL, NULL, &plat.heap, &sz);
    // printf("Opened file: %s \n", f);
    // cfFree(&plat.heap, f, sz);

    // cfAllocatorStats s = cfAllocStats(&plat.heap);
    // CF_ASSERT(s.count == 0, "LEAK!");
    // CF_ASSERT(s.size == 0, "LEAK!");

    // cfPlatformShutdown(&plat);

    return 0;
}
