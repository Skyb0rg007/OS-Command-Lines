
#include <windows.h>
#include <winternl.h>
#include <stdarg.h>

/* Custom fwprintf implementation that escapes strings */
static int my_fwprintf(void *handle, _Printf_format_string_ const wchar_t *format, ...);
/* Implements NtCurrentTeb() */
_Ret_notnull_ static TEB *get_teb(void);

int __stdcall _start(void)
{
    TEB *teb = get_teb();
    PEB *peb = teb->ProcessEnvironmentBlock;
    RTL_USER_PROCESS_PARAMETERS *params = peb->ProcessParameters;
    wchar_t *lpCmdLine = params->CommandLine.Buffer;
    /* The environment pointer is undocumented,
     * but is located directly after the CommandLine field
     * This is the same as GetEnvironmentStringsW, though the Win32 function
     * performs a copy and needs to be free-d */
    wchar_t *lpEnv = *(wchar_t **)(
            (char *)&params->CommandLine + sizeof(params->CommandLine));

    /* Need to use kernel32.lib functions to write to a console */
    int consoleSpawned = AttachConsole(ATTACH_PARENT_PROCESS);
    void *stdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    /* cmd.exe prints a new prompt so we need this to start on a new line */
    my_fwprintf(stdOut, L"\r\n");

    my_fwprintf(stdOut, L"lpCmdLine = %s\r\n", lpCmdLine);

    {
        int argc;
        wchar_t **argv = CommandLineToArgvW(lpCmdLine, &argc);

        my_fwprintf(stdOut, L"argc = %d\r\n", argc);
        for (int i = 0; i < argc; i++) {
            my_fwprintf(stdOut, L"argv[%d] = %s\r\n", i, argv[i]);
        }

        LocalFree(argv);
    }

    {
        wchar_t *p = lpEnv;
        for (int i = 0; *p != L'\0'; i++) {
            my_fwprintf(stdOut, L"lpEnv[%d] = %s\r\n", i, p);
            while (*p)
                p++;
            p++;
        }
    }

    ExitProcess(0);
}

/* Inlined from NtCurrentTeb */
_Ret_notnull_
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

static int my_fwprintf(void *handle, _Printf_format_string_ const wchar_t *format, ...)
{
#define my_fwprintf_bufsize 2048
    unsigned long n = 0;
    wchar_t c;
    int i;
    const wchar_t *s;
    va_list args;
    static wchar_t buf[my_fwprintf_bufsize];

#define my_fwprintf_putc(c)                       \
    do {                                         \
        buf[n++] = (c);                          \
        if (n >= my_fwprintf_bufsize) goto error; \
    } while (0)

    va_start(args, format);

    while ((c = *format++)) {
        if (c != L'%') {
            my_fwprintf_putc(c);
            continue;
        }
        switch ((c = *format++)) {
            case L'd':
                i = va_arg(args, int);
                if (i < 0) {
                    my_fwprintf_putc(L'-');
                }
                if (i == 0) {
                    my_fwprintf_putc(L'0');
                } else {
                    unsigned d = 1;
                    while (i / d >= 10)
                        d *= 10;

                    while (d != 0) {
                        int digit = i / d;
                        i %= d;
                        d /= 10;
                        if (digit > 0 || d == 0)
                            my_fwprintf_putc(L'0' + digit);
                    }
                }
                break;
            case L's':
                s = va_arg(args, const wchar_t *);
                if (s == NULL) {
                    my_fwprintf_putc(L'N');
                    my_fwprintf_putc(L'U');
                    my_fwprintf_putc(L'L');
                    my_fwprintf_putc(L'L');
                    break;
                }
                my_fwprintf_putc(L'\"');
                while ((c = *s++)) {
                    /* TODO: Handle non-printable characters */
                    if (c == L'\"' || c == L'\\')
                        my_fwprintf_putc(L'\\');
                    my_fwprintf_putc(c);
                }
                my_fwprintf_putc(L'\"');
                break;
            case L'%':
                my_fwprintf_putc(L'%');
                break;
            default:
                goto error;
        }
    }

    va_end(args);
    WriteConsoleW(handle, buf, n, NULL, NULL);
    if (n > (unsigned long)INT_MAX)
        return INT_MAX;
    return (int)n;

error:
    va_end(args);
    return -1;

#undef my_fwprintf_putc
#undef my_fwprintf_bufsize
}

