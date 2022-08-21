#include "foundation/win32.inl"

#include <sql.h>
#include <sqlext.h>
#pragma comment(lib, "odbc32.lib")

// TODO (Matteo): Replace with platform API
#include <stdio.h>

typedef struct Platform Platform;
typedef struct CommandLine CommandLine;

CF_INTERNAL bool
sqlHandleResult(SQLRETURN code, I16 handle_type, SQLHANDLE handle)
{
    if (code == SQL_SUCCESS) return true;

    I16 record = 1;

    WCHAR state[6];
    WCHAR message[1024];

    while (SQLGetDiagRecW(handle_type, handle, record++, state, NULL, message,
                          CF_ARRAY_SIZE(message), NULL) == SQL_SUCCESS)
    {
        wprintf(L"SQL ERROR %s - %s", state, message);
    }

    return (code != SQL_ERROR);
}

CF_INTERNAL SQLHENV
sqlEnvCreate(void)
{
    SQLHENV env = SQL_NULL_HANDLE;
    SQLRETURN code = SQL_SUCCESS;

    // Create environment handle
    code = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    if (!sqlHandleResult(code, SQL_HANDLE_ENV, SQL_NULL_HANDLE)) return SQL_NULL_HANDLE;

    // Require ODBC version 3.0
    code = SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    if (!sqlHandleResult(code, SQL_HANDLE_ENV, env)) return SQL_NULL_HANDLE;

    return env;
}

CF_INTERNAL void
sqlEnvDestroy(SQLHENV env)
{
    SQLFreeEnv(env);
}

I32
consoleMain(Platform *platform, CommandLine *cmd_line)
{
    CF_UNUSED(platform);
    CF_UNUSED(cmd_line);

    SQLRETURN code = SQL_SUCCESS;

    SQLHENV env = sqlEnvCreate();
    if (env == SQL_NULL_HANDLE) return -1;

    WCHAR driver_desc[1024];
    WCHAR driver_attr[1024];

    U16 direction = SQL_FETCH_FIRST;
    while (true)
    {
        code = SQLDriversW(env, direction,                                //
                           driver_desc, CF_ARRAY_SIZE(driver_desc), NULL, //
                           driver_attr, CF_ARRAY_SIZE(driver_attr), NULL);

        if (code == SQL_NO_DATA) break;

        if (!sqlHandleResult(code, SQL_HANDLE_ENV, env)) return -1;

        wprintf(L"%s\n", driver_desc);
        direction = SQL_FETCH_NEXT;
    }

    sqlEnvDestroy(env);

    return 0;
}
