/* Common reset entry for Raspberry Pi 3B and 5. */
.section .text.boot, "ax"
.global _start
.type _start, %function
_start:
    msr daifset, #15
    mrs x1, mpidr_el1
    and x1, x1, #0xffff
    cbnz x1, .Lpark
    msr spsel, #1
    ldr x1, =__stack_top
    mov sp, x1
    mrs x1, CurrentEL
    ldr x2, =rei_vectors
    cmp x1, #12
    b.ne .Lvector_el2
    msr vbar_el3, x2
    b .Lvector_ready
.Lvector_el2:
    cmp x1, #8
    b.ne .Lvector_el1
    msr vbar_el2, x2
    b .Lvector_ready
.Lvector_el1:
    msr vbar_el1, x2
.Lvector_ready:
    isb
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

.section .text.rei_vectors, "ax"
.balign 2048
rei_vectors:
.rept 16
    b .Lexception
    .space 124
.endr
.Lexception:
    mrs x3, CurrentEL
    cmp x3, #12
    b.eq .Lexception_el3
    cmp x3, #8
    b.eq .Lexception_el2
    mrs x0, esr_el1
    mrs x1, elr_el1
    mrs x2, far_el1
    b rei_exception
.Lexception_el2:
    mrs x0, esr_el2
    mrs x1, elr_el2
    mrs x2, far_el2
    b rei_exception
.Lexception_el3:
    mrs x0, esr_el3
    mrs x1, elr_el3
    mrs x2, far_el3
    b rei_exception
