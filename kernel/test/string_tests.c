#include "test.h"
#include "string.h"
#include "util.h"

static void test_strncpy_copy(void)
{
    char dest[16];

    strncpy(dest, "hello", sizeof(dest));

    TEST_ASSERT_EQ(strcmp(dest, "hello"), 0);

    test_pass();
}

static test_entry_t tests[] =
{
    { "strncpy_copy", test_strncpy_copy },
};

void test_string(void)
{
    uint32_t i;

    test_begin("String");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        test_case(tests[i].name);

        tests[i].func();
    }

    test_end();
}
