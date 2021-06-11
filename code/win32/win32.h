// Attempt to include windows headers in a clean way

#pragma once

#pragma warning(push)
#pragma warning(disable : 5105)

#define NOMINMAX 1
#define VC_EXTRALEAN 1
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
// Must be included AFTER <windows.h>
#include <commdlg.h>
#include <process.h>
#include <shellapi.h>

#pragma warning(pop)
