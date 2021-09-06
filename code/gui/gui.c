#include "gui.h"

#include "foundation/array.h"
#include "foundation/strings.h"

// Restore warnings disabled for DearImgui compilation
#if CF_COMPILER_CLANG
#    pragma clang diagnostic warning "-Wsign-conversion"
#    pragma clang diagnostic warning "-Wimplicit-int-float-conversion"
#    pragma clang diagnostic warning "-Wunused-function"
#    pragma clang diagnostic warning "-Wfloat-conversion"
#elif CF_COMPILER_MSVC
#endif

#if CF_OS_WIN32
#    include "foundation/win32.h"

typedef BOOL(APIENTRY *Win32FileDialog)(LPOPENFILENAMEW);

static const Win32FileDialog win32FileDialog[2] = {
    [GuiFileDialog_Open] = GetOpenFileNameW,
    [GuiFileDialog_Save] = GetSaveFileNameW,
};

static const DWORD win32FileDialogFlags[2] = {
    [GuiFileDialog_Open] = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST,
    [GuiFileDialog_Save] = OFN_OVERWRITEPROMPT,
};

static StrBuf16
win32BuildFilterString(GuiFileDialogFilter *filters, Usize num_filters, MemAllocator alloc)
{
    StrBuf16 out_filter = {0};
    cfArrayInitCap(&out_filter, alloc, 1024);

    if (num_filters == 0) return out_filter;

    for (GuiFileDialogFilter *filter = filters, *end = filter + num_filters; //
         filter < end; ++filter)
    {
        Str filter_name = strFromCstr(filter->name);
        Usize name_size = win32Utf8To16(filter_name, NULL, 0) + 1;

        cfArrayReserve(&out_filter, name_size);
        win32Utf8To16(filter_name, cfArrayEnd(&out_filter), name_size);
        cfArrayExtend(&out_filter, name_size);

        for (Usize ext_no = 0; ext_no < filter->num_extensions; ++ext_no)
        {
            Str ext = strFromCstr(filter->extensions[ext_no]);
            Usize ext_size = win32Utf8To16(ext, NULL, 0) + 1;

            // Prepend '*' to the extension - not documented but actually required
            cfArrayPush(&out_filter, L'*');
            cfArrayReserve(&out_filter, ext_size);
            win32Utf8To16(ext, cfArrayEnd(&out_filter), ext_size);
            cfArrayExtend(&out_filter, ext_size);

            // Replace null terminator with ';' to separate extensions
            *cfArrayLast(&out_filter) = L';';
        }

        // Append 2 null terminators (required since null terminators are used
        // internally to separate filters)
        cfArrayPush(&out_filter, 0);
        cfArrayPush(&out_filter, 0);
    }

    return out_filter;
}

GuiFileDialogResult
guiFileDialog(GuiFileDialogParms *parms, MemAllocator alloc)
{
    GuiFileDialogResult result = {.code = GuiFileDialogResult_Error};

    Char16 name[MAX_PATH] = {0};

    if (strValid(parms->filename_hint))
    {
        Usize name_length = win32Utf8To16(parms->filename_hint, NULL, 0);
        if (name_length >= MAX_PATH) return result;
        win32Utf8To16(parms->filename_hint, name, name_length);
    }

    StrBuf16 filt = win32BuildFilterString(parms->filters, parms->num_filters, alloc);

    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = name;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filt.buf; // L"Image files\0*.jpg;*.jpeg;*.bmp;*.png\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = win32FileDialogFlags[parms->type];

    if (win32FileDialog[parms->type](&ofn))
    {
        Str16 filename16 = str16FromCstr(ofn.lpstrFile);

        result.filename.len = win32Utf16To8(filename16, NULL, 0);
        result.filename.buf = (Char8 *)memAlloc(alloc, result.filename.len);

        if (result.filename.buf)
        {
            result.code = GuiFileDialogResult_Ok;
            win32Utf16To8(filename16, (Char8 *)result.filename.buf, result.filename.len);
        }
        else
        {
            result.filename.len = 0;
        }
    }
    else
    {
        result.code = GuiFileDialogResult_Cancel;
    }

    cfArrayFree(&filt);

    return result;
}

#endif
