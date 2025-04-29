__attribute__((noreturn))
void user_shell_main() {
    while (1) {
        asm volatile("li a7, 4\n"
                     "ecall\n");
    }
}
