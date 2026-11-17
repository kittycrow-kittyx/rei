ENTRY(_start)
SECTIONS {
    . = 0x80000;
    .text : ALIGN(16) { KEEP(*(.text.boot)) *(.text .text.*) }
    .rodata : ALIGN(16) { *(.rodata .rodata.*) }
    .data : ALIGN(16) { *(.data .data.*) }
    .bss (NOLOAD) : ALIGN(16) {
        __bss_start = .; *(.bss .bss.*) *(COMMON)
        . = ALIGN(16); __bss_end = .;
    }
    .stack (NOLOAD) : ALIGN(16) { . += 0x20000; __stack_top = .; }
    .arena 0x04000000 (NOLOAD) : ALIGN(4096) { *(.arena) }
    ASSERT(__stack_top < 0x04000000, "Kernel and embedded root overlap arena")
    ASSERT(SIZEOF(.arena) <= 0x05000000, "Native allocation pool too large")
    /DISCARD/ : { *(.comment*) *(.note*) *(.eh_frame*) }
}
