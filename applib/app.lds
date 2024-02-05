ENTRY(_start)
SECTIONS
{
    . = 0x41000000;

    .text : {
        *(.text)
    }
    .rodata : {
        *(.rodata)
    }
    . = ALIGN(4096);
    .data : {
        *(.data)
    }
    .bss : {
        bss_start = .;
        *(*.bss)
        . = ALIGN(4096);
        *(.stack)
        bss_end = .;
    }
}