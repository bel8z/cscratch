#include <windows.h>

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

typedef struct FileInfo
{
    char path[MAX_PATH];
    usize size;
} FileInfo;

typedef struct DirectoryInfo
{
    char path[MAX_PATH];
} DirectoryInfo;

typedef struct FsIter
{
    WIN32_FIND_DATAA data;
    HANDLE finder;
    bool state;
} FsIter;

void
fs_end(FsIter *iter)
{
    assert(iter);

    if (iter->finder != INVALID_HANDLE_VALUE)
    {
        FindClose(iter->finder);
    }
}

bool
fs_start(FsIter *iter, char const *path)
{
    assert(iter);

    if (!path) return false;

    char local[MAX_PATH];
    if (strncpy_s(local, MAX_PATH, path, MAX_PATH)) return false;
    if (strncat_s(local, MAX_PATH, "\\*", 2)) return false;

    *iter = (FsIter){0};

    iter->finder = FindFirstFileA("C:/*", &iter->data);
    iter->state = (iter->finder != INVALID_HANDLE_VALUE);

    return iter->state;
}

bool
fs_next_file(FsIter *iter, FileInfo *file)
{
    assert(iter);
    assert(file);

    while (iter->state && iter->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        iter->state = FindNextFileA(iter->finder, &iter->data);
    }

    if (iter->state)
    {
        LARGE_INTEGER file_size = {0};
        file_size.LowPart = iter->data.nFileSizeLow;
        file_size.HighPart = iter->data.nFileSizeHigh;

        file->size = file_size.QuadPart;
        strncpy_s(file->path, MAX_PATH, iter->data.cFileName, MAX_PATH);

        iter->state = FindNextFileA(iter->finder, &iter->data);
    }

    return iter->state;
}

bool
fs_next_directory(FsIter *iter, DirectoryInfo *dir)
{
    assert(iter);
    assert(dir);

    while (iter->state && !(iter->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        iter->state = FindNextFileA(iter->finder, &iter->data);
    }

    if (iter->state)
    {
        strncpy_s(dir->path, MAX_PATH, iter->data.cFileName, MAX_PATH);
        iter->state = FindNextFileA(iter->finder, &iter->data);
    }

    return iter->state;
}

int
main()
{
    FsIter iter;
    FileInfo info;
    DirectoryInfo dir;

    if (fs_start(&iter, "C:/"))
    {
        printf("------------\n");
        printf("C:/ Files   \n");
        printf("------------\n");

        while (fs_next_file(&iter, &info))
        {
            printf("%s", info.path);
            printf(" %zu bytes", info.size);
            printf("\n");
        }

        fs_end(&iter);
    }

    if (fs_start(&iter, "C:/"))
    {
        printf("------------\n");
        printf("C:/ Folders \n");
        printf("------------\n");

        while (fs_next_directory(&iter, &dir))
        {
            printf("%s", dir.path);
            printf("\n");
        }

        fs_end(&iter);
    }

    printf("\n");
    printf("\n");
    printf("\n");
    printf("sizeof(bool) = %zu", sizeof(bool));

    return 0;
}