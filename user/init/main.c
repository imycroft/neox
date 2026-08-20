#include "unistd.h"

void _start(void)
{


    static char arg0[] = "/sbin/init";


    static const char message[] =
    "[NEOX] The End\n";


    // static char arg1[] = "hello";
    // static char arg2[] = "world";

    static char *argv[] =
    {
        arg0,
        // arg1,
        // arg2,
        NULL
    };

    int result;

    ssize_t count =  write(
        1,
        message,
        sizeof(message) - 1
    );

    if (count < 0)
    {
        exit(1);
    }

    result = exec(
        "/sbin/hello",
        argv
    );



    /*
     * exec() should eventually never return on success.
     * For now our kernel implementation returns -1.
     */
    if (result < 0)
        exit(1);

    exit(0);
}
