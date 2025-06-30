ENTRY(_entry)

SECTIONS
{
    . = 0xffffffff80000000;  /* Starting address of the kernel */

    .text : AT(0x80000000) {
        _kernel_text_start = .;
        *(.text .text.*)  /* All code goes here */
        _kernel_text_end = .;
    }

    .rodata : ALIGN(0x1000) {
        _kernel_rodata_start = .;
        *(.rodata .rodata.*)  /* Read-only data goes here */
        _kernel_rodata_end = .;
    }

    .data : ALIGN(0x1000) {
        _kernel_data_start = .;
        *(.*data .*data.*)  /* All initialized data goes here */
    }

    .bss : ALIGN(0x1000) {
        *(.*bss .*bss.*)  /* All uninitialized data goes here */
    }

    _kernel_end = .;
}
