#ifndef ASSERT_H
#define ASSERT_H

/*
 * Halt the kernel if an assertion fails.
 */
void assert_fail(const char *expr,
                 const char *file,
                 int line,
                 void *caller);

/*
 * Kernel assertion.
 */
#define ASSERT(expr)                                   \
do                                                     \
{                                                      \
    if (!(expr))                                       \
    {                                                  \
        assert_fail(                                   \
        #expr,                                         \
        __FILE__,                                      \
        __LINE__,                                      \
        __builtin_return_address(0)                    \
        );                                             \
    }                                                  \
} while (0)

#endif
