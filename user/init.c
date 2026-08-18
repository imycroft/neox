#define SYS_EXIT  1
#define SYS_WRITE 4

typedef unsigned int uintptr_t;

static uintptr_t syscall2(
    uintptr_t number,
    uintptr_t arg1,
    uintptr_t arg2)
{
    uintptr_t result;

    __asm__ volatile (
        "int $0x80"
        : "=a"(result)
        : "a"(number),
         "b"(arg1),
         "c"(arg2)
        : "memory"
    );

    return result;
}

void _start(void)
{
    static const char message[] =
    "Hello from init!\n";

        syscall2(
            SYS_WRITE,
            (uintptr_t)message,
                 sizeof(message) - 1
        );

        syscall2(
            SYS_EXIT,
            0,
            0
        );

        for (;;)
            ;
}
