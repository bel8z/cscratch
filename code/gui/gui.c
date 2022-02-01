#include "gui.h"

#include "foundation/math.inl"
#include "foundation/mem_array.inl"
#include "foundation/memory.h"
#include "foundation/strings.h"

// Restore warnings disabled for DearImgui compilation
CF_DIAGNOSTIC_IGNORE_CLANG("-Wsign-conversion")

// Include stb implementation
CF_DIAGNOSTIC_PUSH()
CF_DIAGNOSTIC_IGNORE_CLANG("-Wsign-conversion")

#include <stdlib.h>

#define STBRP_ASSERT(x) CF_ASSERT(x, "[STBRP] assertion failed")
#define STBRP_SORT qsort
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#define STBTT_assert(x) CF_ASSERT(x, "[STBTT] assertion failed")
#define STBTT_memcpy(dst, src, size) memCopy(src, dst, size)
#define STBTT_memset(dst, val, size) memWrite((U8 *)dst, val, size)
#define STBTT_sqrt mSqrt
#define STBTT_pow mPow
#define STBTT_fmod mFmod
#define STBTT_fabs mAbs
#define STBTT_cos(x) mCos(x)
#define STBTT_acos(x) mAcos(x)
#define STBTT_ifloor(x) ((int)mFloor(x))
#define STBTT_iceil(x) ((int)mCeil(x))
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

CF_DIAGNOSTIC_POP()

static GuiFont *
gui_LoadCustomFont(GuiFontAtlas *fonts, Str data_path, Cstr name, F32 font_size)
{
    Char8 buffer[1024] = {0};
    strPrint(buffer, CF_ARRAY_SIZE(buffer), "%.*s%s.ttf", (I32)data_path.len, data_path.buf, name);
    return guiLoadFont(fonts, buffer, font_size);
}

bool
guiLoadCustomFonts(GuiFontAtlas *atlas, F32 dpi_scale, Str data_path)
{
    // TODO (Matteo): Make font list available to the application?

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can
    // also load multiple fonts and use igPushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you
    // need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please
    // handle those errors in your application (e.g. use an assertion, or
    // display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and
    // stored into a texture when calling
    // GuiFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame
    // below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string
    // literal you need to write a double backslash \\ !

    F32 const scale = dpi_scale * GUI_PLATFORM_DPI / GUI_TRUETYPE_DPI;

#if CF_OS_WIN32
    Str const system_path = strLiteral("C:/Windows/Fonts/");
#endif

    // NOTE (Matteo): This ensure the proper loading order even in optimized release builds
    GuiFont const *fonts[] = {
#if CF_OS_WIN32
        gui_LoadCustomFont(atlas, system_path, "SegoeUI", mRound(14.0f * scale)),
        gui_LoadCustomFont(atlas, system_path, "Arial", mRound(12.0f * scale)),
#endif
        gui_LoadCustomFont(atlas, data_path, "NotoSans", mRound(13.5f * scale)),
        gui_LoadCustomFont(atlas, data_path, "DroidSans", mRound(12.0f * scale)),
        gui_LoadCustomFont(atlas, data_path, "SourceSansPro", mRound(14.0f * scale)),
    };

    for (Usize i = 0; i < CF_ARRAY_SIZE(fonts); ++i)
    {
        if (fonts[i]) return true;
    }

    return false;
}

#if CF_OS_WIN32
#    include "foundation/win32.inl"

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
    memArrayInitCap(&out_filter, alloc, 1024);

    if (num_filters == 0) return out_filter;

    for (GuiFileDialogFilter *filter = filters, *end = filter + num_filters; //
         filter < end; ++filter)
    {
        Str filter_name = strFromCstr(filter->name);
        Usize name_size = win32Utf8To16(filter_name, NULL, 0) + 1;

        memArrayReserve(&out_filter, name_size);
        win32Utf8To16(filter_name, memArrayEnd(&out_filter), name_size);
        memArrayExtend(&out_filter, name_size);

        for (Usize ext_no = 0; ext_no < filter->num_extensions; ++ext_no)
        {
            Str ext = strFromCstr(filter->extensions[ext_no]);
            Usize ext_size = win32Utf8To16(ext, NULL, 0) + 1;

            // Prepend '*' to the extension - not documented but actually required
            memArrayPush(&out_filter, L'*');
            memArrayReserve(&out_filter, ext_size);
            win32Utf8To16(ext, memArrayEnd(&out_filter), ext_size);
            memArrayExtend(&out_filter, ext_size);

            // Replace null terminator with ';' to separate extensions
            *memArrayLast(&out_filter) = L';';
        }

        // Append 2 null terminators (required since null terminators are used
        // internally to separate filters)
        memArrayPush(&out_filter, 0);
        memArrayPush(&out_filter, 0);
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
    ofn.lpstrFilter = filt.data; // L"Image files\0*.jpg;*.jpeg;*.bmp;*.png\0";
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

    memArrayShutdown(&filt);

    return result;
}

#endif
