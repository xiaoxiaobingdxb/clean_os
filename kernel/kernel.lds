SECTIONS
{
    PROVIDE(kernel_base = 0x0);

    . = 0x10000;

    PROVIDE(s_text = .);
    .text : {
        *(.text)
    }
    .rodata : {
        *(.rodata)
    }
    PROVIDE(e_text = .);

    . = ALIGN(4096);
    PROVIDE(s_data = .);
    .data : {
        *(.data)
    }
    .bss : {
        *(.bss)
    }
    PROVIDE(e_data = .);

    PROVIDE(mem_free_start = e_data);
}