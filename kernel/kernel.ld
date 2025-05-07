ENTRY(_entry)

SECTIONS
{
    . = 0x80000000;  /* Starting address of the kernel */

    .text : {
        *(.text)  /* All code goes here */
    }

    .data : {
        *(.data)  /* All initialized data goes here */
    }

    .bss : {
        *(.bss)  /* All uninitialized data goes here */
    }

    /* Define symbol _kernel_end at current location */
    _kernel_end = .;
}
