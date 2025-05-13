#include "user.h"

#define INPUT_BUF_SIZE 128

__attribute__((section(".text.user_shell_main"), used))
void
user_shell_main()
{
    char buf[INPUT_BUF_SIZE] = "$ ";

    /*
    volatile char *uart = (char *)0x10000000;
    char msg[] = {'H', '\n', 0};
    for (int i = 0; msg[i] != 0; i++) {
        uart[0] = msg[i];
    }
    */

    while (1) {
        // Print prompt
        const char prompt[] = "$ ";
        write(prompt, 2);

        // Read input (blocking)
        int n = read(&buf[2], sizeof(buf) - 1);
        if (n <= 0)
            continue;

        sleep(1000);

        // Echo back
        write(buf, n);

        yield();
    }
}
