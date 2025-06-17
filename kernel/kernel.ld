ENTRY(_entry)

SECTIONS
{
    . = 0xffffffff80000000;  /* Starting address of the kernel */

    .text : AT(0x80000000) {
        _kernel_start = .;
        *(.text .text.*)  /* All code goes here */
    }

    .rodata : ALIGN(16) {
        *(.rodata .rodata.*)  /* Read-only data goes here */
    }

    .data : ALIGN(16) {
        *(.data .data.*)  /* All initialized data goes here */
    }

    .bss : ALIGN(16) {
        *(.bss .bss.*)  /* All uninitialized data goes here */
    }

    /* Define symbol _kernel_end at current location */
    _kernel_end = .;
}
