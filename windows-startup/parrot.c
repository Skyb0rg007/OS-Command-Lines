
#include <windows.h>
#include <winternl.h>

static TEB *get_teb(void);
static void console_log(void *handle, const wchar_t *fmt, ...);

int __stdcall _start(void)
{
    AttachConsole(ATTACH_PARENT_PROCESS);
    void *stdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    TEB *teb = get_teb();
    PEB *peb = teb->ProcessEnvironmentBlock;
    RTL_USER_PROCESS_PARAMETERS *params = peb->ProcessParameters;
    wchar_t *lpCmdLine = params->CommandLine.Buffer;
    wchar_t *lpEnv = *(wchar_t **)(params + 1);

    console_log(stdOut, L"\r\n");

    console_log(stdOut, L"lpCmdLine = \"%1!s!\"\r\n", lpCmdLine);

    {
        int argc;
        wchar_t **argv = CommandLineToArgvW(lpCmdLine, &argc);
        wchar_t *lpEnv = GetEnvironmentStringsW();

        console_log(stdOut, L"argc = %1!d!\r\n", argc);
        for (int i = 0; i < argc; i++) {
            console_log(stdOut, L"argv[%1!d!] = \"%2!s!\"\r\n", i, argv[i]);
        }

        LocalFree(argv);
    }

    {
        wchar_t *p = lpEnv;
        for (int i = 0; *p != L'\0'; i++) {
            console_log(stdOut, L"lpEnv[%1!d!] = \"%2!s!\"\r\n", i, p);
            p += lstrlenW(p) + 1;
        }
    }

    ExitProcess(0);
}

static void console_log(void *handle, const wchar_t *fmt, ...)
{
    static wchar_t buf[2048];
    unsigned long len;
    va_list args;
    va_start(args, fmt);

    len = FormatMessage(
            FORMAT_MESSAGE_FROM_STRING,
            fmt, 0, 0, buf, 2048, &args);
    va_end(args);

    WriteConsoleW(handle, buf, len, NULL, NULL);
}

// Inlined from winnt.h
static TEB *get_teb(void) {
#if defined(_M_AMD64) && !defined(_M_ARM64EC) && !defined(__midl)
    return (TEB *)__readgsqword(offsetof(NT_TIB, Self));
#elif defined(_M_ARM) && !defined(__midl) && !defined(_M_CEE_PURE)
    return (TEB *)(uintptr_t)_MoveFromCoprocessor(CP15_TPIDRURW);
#elif (defined(_M_ARM64) || defined(_M_ARM64EC)) && !defined(__midl) && !defined(_M_CEE_PURE)
    return (TEB *)__getReg(18);
#elif defined(_M_IX86) && !defined(MIDL_PASS) && !defined(_M_CEE_PURE)
    return (TEB *)(uintptr_t)__readfsdword(0x18);
#else
#  error Unsupported platform
#endif
}


