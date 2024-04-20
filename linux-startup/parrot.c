#include <unistd.h>
#include <sys/syscall.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <limits.h>

static int my_fprintf(uint64_t fd, const char *format, ...) __attribute__((__nonnull__(2), __format__(__printf__, 2, 3)));
static int64_t sys_write(uint64_t fd, const char *buf, uint64_t count) __attribute__((__nonnull__(2)));
static void sys_exit(uint64_t code) __attribute__((__noreturn__));

/* The arguments are determined by the initial value of rsp,
 * but the standard function prologue modifies it.
 * We use `naked` and inline assembly to convert the argument from
 * a stack-passed argument to a register argument.
 */
__attribute__((__naked__, __noreturn__))
void _start(void) {
    __asm__ __volatile__(
        "mov %%rsp, %%rdi\n"
        "jmp _start_c"
        : );
}

__attribute__((__used__, __noreturn__))
static void _start_c(uint64_t *rsp) {
    int argc = (int)rsp[0];
    char **argv = (char **)&rsp[1];
    char **envp = (char **)&rsp[1 + argc + 1];

    for (int i = 0; i < argc; i++) {
        my_fprintf(STDOUT_FILENO, "argv[%d] = %s\n", i, argv[i]);
    }

    my_fprintf(STDOUT_FILENO, "-----------------------\n");

    for (int i = 0; envp[i] != NULL; i++) {
        my_fprintf(STDOUT_FILENO, "envp[%d] = %s\n", i, envp[i]);
    }
    sys_exit(0);
}

static int64_t sys_write(uint64_t fd, const char *buf, uint64_t count)
{
    uint64_t bytes;
    __asm__ __volatile__(
        "syscall"
        : "=a"(bytes)
        : "a"(SYS_write), "D"(fd), "S"(buf), "d"(count)
        : "rcx", "r11", "memory"
        );
    return bytes;
}

static void sys_exit(uint64_t code)
{
    __asm__ __volatile__(
        "syscall"
        :
        : "a"(SYS_exit_group), "D"(code));
    __builtin_unreachable();
}

static int my_fprintf(uint64_t fd, const char *format, ...)
{
#define my_fprintf_bufsize 2048
    uint64_t n = 0;
    char c;
    int i;
    const char *s;
    va_list args;
    static char buf[my_fprintf_bufsize];

#define my_fprintf_putc(c)             \
    do {                              \
        buf[n++] = (c);               \
        if (n >= my_fprintf_bufsize) { \
            n = my_fprintf_bufsize;    \
            goto done;                \
        }                             \
    } while (0)

    va_start(args, format);

    while ((c = *format++)) {
        if (c != '%') {
            my_fprintf_putc(c);
            continue;
        }
        switch ((c = *format++)) {
            case 'd':
                i = va_arg(args, int);
                if (i < 0) {
                    my_fprintf_putc('-');
                }
                if (i == 0) {
                    my_fprintf_putc('0');
                } else {
                    unsigned d = 1;
                    while (i / d >= 10)
                        d *= 10;

                    while (d != 0) {
                        int digit = i / d;
                        i %= d;
                        d /= 10;
                        if (digit > 0 || d == 0)
                            my_fprintf_putc('0' + digit);
                    }
                }
                break;
            case 's':
                s = va_arg(args, const char *);
                if (s == NULL) {
                    my_fprintf_putc('N');
                    my_fprintf_putc('U');
                    my_fprintf_putc('L');
                    my_fprintf_putc('L');
                    break;
                }
                my_fprintf_putc('\"');
                while ((c = *s++)) {
                    /* TODO: Handle non-printable characters */
                    if (c == '\"' || c == '\\')
                        my_fprintf_putc('\\');
                    my_fprintf_putc(c);
                }
                my_fprintf_putc('\"');
                break;
            case '%':
                my_fprintf_putc('%');
                break;
            default:
                goto error;
        }
    }

done:
    va_end(args);
    sys_write(fd, buf, n);
    if (n > (uint64_t)INT_MAX)
        return INT_MAX;
    return (int)n;

error:
    va_end(args);
    return -1;

#undef my_fprintf_putc
#undef my_fprintf_bufsize
}
