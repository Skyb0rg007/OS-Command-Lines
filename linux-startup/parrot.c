#include <unistd.h>
#include <sys/syscall.h>
#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <limits.h>

static int my_snprintf(char *str, size_t size, const char *format, ...) __attribute__((__nonnull__(1, 3), __format__(__printf__, 3, 4)));
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

    static char buf[2048];
    int len;

    for (int i = 0; i < argc; i++) {
        len = my_snprintf(buf, sizeof buf, "argv[%d] = \"%s\"\n", i, argv[i]);
        sys_write(STDOUT_FILENO, buf, len);
    }

    len = my_snprintf(buf, sizeof buf, "------------------------------\n");
    sys_write(STDOUT_FILENO, buf, len);

    for (int i = 0; envp[i] != NULL; i++) {
        len = my_snprintf(buf, sizeof buf, "envp[%d] = \"%s\"\n", i, envp[i]);
        sys_write(STDOUT_FILENO, buf, len);
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

static int my_snprintf(char *str, size_t size, const char *format, ...)
{
    size_t n = 0;
    char c;
    va_list args;

    if (size >= (size_t)INT_MAX)
        return -1;

    va_start(args, format);

    while (n < size && (c = *format++)) {
        if (c != '%') {
            str[n++] = c;
        } else {
            switch (*format++) {
                case 'd': {
                    int i = va_arg(args, int);
                    if (i < 0) {
                        str[n++] = '-';
                        if (n == size)
                            goto end;
                    }
                    if (i == 0) {
                        str[n++] = '0';
                    } else {
                        unsigned d = 1;
                        while (i / d >= 10)
                            d *= 10;

                        while (d != 0) {
                            int digit = i / d;
                            i %= d;
                            d /= 10;
                            if (digit > 0 || d == 0) {
                                str[n++] = '0' + digit;
                                if (n == size)
                                    goto end;
                            }
                        }
                    }

                    break;
                }
                case 's': {
                    const char *s = va_arg(args, const char *);
                    if (s == NULL)
                        s = "(null)";
                    while (*s) {
                        if (n == size) goto end;
                        str[n++] = *s++;
                    }
                    break;
                }
                default:
                    return -1;
            }
        }
    }

    va_end(args);
    n++;

end:
    str[n - 1] = '\0';
    return (int)n;
}

