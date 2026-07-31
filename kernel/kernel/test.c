#include "test.h"
#include "vam.h"

#include "printf.h"

void kernel_tests(void)
{
    void *a;
    void *b;
    void *c;

    a = vam_alloc_pages(1);
    b = vam_alloc_pages(2);

    vam_free_pages((uintptr_t)b, 2);

    c = vam_alloc_pages(2);

    printf("A = %x\n", (uint32_t)a);
    printf("B = %x\n", (uint32_t)b);
    printf("C = %x\n", (uint32_t)c);
}
