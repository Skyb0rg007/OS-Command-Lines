#include <unistd.h>
#include <sys/syscall.h>
#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>

static size_t my_strlen(const char *str);
static int64_t sys_write(uint64_t fd, const char *buf, uint64_t count);
static noreturn void sys_exit(uint64_t code);

static const int MAX_ENV = 4;

/* The arguments are determined by the initial value of rsp,
 * so use `naked` and inline assembly to extract it before it's changed. */
__attribute__((naked))
noreturn void _start(void) {
    __asm__ __volatile__(
        "mov %%rsp, %%rdi\n"
        "jmp start"
        : );
}

__attribute__((used))
static noreturn void start(uint64_t *rsp) {
    int argc = (int)rsp[0];
    char **argv = (char **)&rsp[1];
    char **envp = (char **)&rsp[1 + argc + 1];

    for (int i = 0; i < 10 && i < argc; i++) {
        sys_write(STDOUT_FILENO, "argv[", 5);
        sys_write(STDOUT_FILENO, &(char){'0' + i}, 1);
        sys_write(STDOUT_FILENO, "] = \"", 5);
        sys_write(STDOUT_FILENO, argv[i], my_strlen(argv[i]));
        sys_write(STDOUT_FILENO, "\"\n", 2);
    }

    for (int i = 0; i < MAX_ENV && envp[i] != NULL; i++) {
        sys_write(STDOUT_FILENO, envp[i], my_strlen(envp[i]));
        sys_write(STDOUT_FILENO, "\n", 1);
    }
    sys_exit(0);
}

static size_t my_strlen(const char *str) {
    size_t n = 0;
    while (*str++)
        n++;
    return n;
}

static int64_t sys_write(uint64_t fd, const char *buf, uint64_t count) {
    uint64_t bytes;
    __asm__ __volatile__(
        "syscall"
        : "=a"(bytes)
        : "a"(SYS_write), "D"(fd), "S"(buf), "d"(count)
        : "rcx", "r11", "memory"
        );
    return bytes;
}

static noreturn void sys_exit(uint64_t code) {
    __asm__ __volatile__(
        "syscall"
        :
        : "a"(SYS_exit_group), "D"(code));
    __builtin_unreachable();
}

