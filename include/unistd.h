#pragma once

#include "syscall_abi.h"

typedef unsigned int uint32_t;
typedef unsigned int uintptr_t;
typedef unsigned int size_t;
typedef int ssize_t;

#ifndef NULL
#define NULL ((void *)0)
#endif


static inline uintptr_t syscall2(
    uintptr_t number,
    uintptr_t arg1,
    uintptr_t arg2
)
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

static inline uintptr_t syscall3(
    uintptr_t number,
    uintptr_t arg1,
    uintptr_t arg2,
    uintptr_t arg3
)
{
    uintptr_t result;

    __asm__ volatile (
        "int $0x80"
        : "=a"(result)
        : "a"(number),
                      "b"(arg1),
                      "c"(arg2),
                      "d"(arg3)
                      : "memory"
    );

    return result;
}

static inline void exit(int status)
{
    syscall2(
        SYS_EXIT,
        (uintptr_t)status,
             0
    );

    for (;;)
        ;
}

static inline ssize_t read(
    int fd,
    void *buffer,
    size_t count
)
{
    return (ssize_t)syscall3(
        SYS_READ,
        (uintptr_t)fd,
                             (uintptr_t)buffer,
                             (uintptr_t)count
    );
}

static inline ssize_t write(
    int fd,
    const void *buffer,
    size_t count
)
{
    return (ssize_t)syscall3(
        SYS_WRITE,
        (uintptr_t)fd,
                             (uintptr_t)buffer,
                             (uintptr_t)count
    );
}

static inline int open(
    const char *path,
    uint32_t access
)
{
    return (int)syscall2(
        SYS_OPEN,
        (uintptr_t)path,
                         (uintptr_t)access
    );
}

static inline int close(int fd)
{
    return (int)syscall2(
        SYS_CLOSE,
        (uintptr_t)fd,
                         0
    );
}

static inline int exec(
    const char *path,
    char *const argv[]
)
{
    return (int)syscall2(
        SYS_EXEC,
        (uintptr_t)path,
        (uintptr_t)argv
    );
}
