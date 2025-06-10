#include "user.h"

#define INPUT_BUF_SIZE 128

int
streq(const char *s1, const char *s2, int n)
{
    for (int i = 0; i < n; i++)
        if (!s1[i] || !s2[i] || s1[i] != s2[i])
            return 0;

    return 1;
}

__attribute__((section(".text.user_shell_main"), used))
void
user_shell_main()
{
    char buf[INPUT_BUF_SIZE] = "$ ";
    const char cmd_yield[] = "yield";

    while (1) {
        // Print prompt
        const char prompt[] = "$ ";
        write(prompt, 2);

        // Read input (blocking)
        int n = read(&buf[2], sizeof(buf) - 1);
        if (n <= 0)
            continue;

        sleep(1000);

        if (streq(&buf[2], cmd_yield, sizeof(cmd_yield) - 1))
            yield();

        // Echo back
        write(buf, n + 2);  // \0 and \n

        //yield();
        //exit();
    }
}
