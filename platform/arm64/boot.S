/* Common reset entry for Raspberry Pi 3B and 5. */
.section .text.boot, "ax"
.global _start
.type _start, %function
_start:
    msr daifset, #15
    mrs x1, mpidr_el1
    and x1, x1, #255
    cbnz x1, .Lpark
    msr spsel, #1
    ldr x1, =__stack_top
    mov sp, x1
    mrs x1, CurrentEL
    cmp x1, #12
    b.ne 1f
    msr cptr_el3, xzr
1:  cmp x1, #8
    b.lo 2f
    msr cptr_el2, xzr
2:  mov x1, #(3 << 20)
    msr cpacr_el1, x1
    isb
    ldr x1, =__bss_start
    ldr x2, =__bss_end
3:  cmp x1, x2
    b.hs 4f
    str xzr, [x1], #8
    b 3b
4:  bl rei_native_start
.Lpark:
    wfe
    b .Lpark
.size _start, .-_start
