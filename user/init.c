void _start(void)
{
    const char msg[] = "[init] Hello from user space!\n";
    unsigned int len = sizeof(msg) - 1;

    __asm__ volatile (
        "mov $4, %%eax\n\t"
        "mov %0, %%ebx\n\t"
        "mov %1, %%ecx\n\t"
        "int $0x80\n\t"
        :
        : "r"(msg), "r"(len)
        : "eax", "ebx", "ecx"
    );

    /*
     * Deliberately generate #UD (Invalid Opcode).
     */
    __asm__ volatile ("ud2");

    /*
     * We should never reach this.
     */
    __asm__ volatile (
        "mov $1, %%eax\n\t"
        "xor %%ebx, %%ebx\n\t"
        "int $0x80\n\t"
        :
        :
        : "eax", "ebx"
    );

    for (;;)
        __asm__ volatile ("hlt");
}
