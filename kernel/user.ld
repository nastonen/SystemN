ENTRY(user_shell_main)

SECTIONS {
  . = 0x84000000;

  .text : {
    PROVIDE(user_shell_main = .);
    *(.text.user_shell_main)
    *(.text*)
  }

  .rodata : {
    *(.rodata*)
  }

  .data : {
    *(.data*)
  }

  .bss : {
    *(.bss*)
    *(COMMON)
  }
}
