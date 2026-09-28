#include "course_sid.h"

        .section .text
        .globl _entry
_entry:
        # No trap handler is ready yet, so keep machine interrupts disabled.
        csrw mie, zero

        # Lab 1 runs only the boot hart. Park every secondary hart.
        csrr t0, mhartid
        bnez t0, park

        # The stack grows downward from the aligned end of boot_stack.
        la sp, boot_stack
        li t0, LAB1_STACK_KB * 1024
        add sp, sp, t0

        # Unexpected early traps must land on an aligned, safe loop.
        la t0, early_trap
        csrw mtvec, t0

        call start

park:
        wfi
        j park

        .balign 4
early_trap:
        wfi
        j early_trap

        .section .bss.stack
        .balign 16
boot_stack:
        .space LAB1_STACK_KB * 1024

