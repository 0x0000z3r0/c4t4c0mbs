.section .text.init
.global _start
.align 4

_start:
    // Disable all interrupts.
    csrw mie, zero

    // Setup stack pointer.
    la sp, _stack_end

    // Clear BSS.
    la t0, _bss_start
    la t1, _bss_end
    bgeu t0, t1, 2f
1:
    sd zero, (t0)
    addi t0, t0, 8
    bltu t0, t1, 1b
2:

    // Setup trap handler.
    la t0, freertos_risc_v_trap_handler
    csrw mtvec, t0

    // Jump to main.
    call main

    // If main returns, loop forever.
    3:
    wfi
    j 3b

.global freertos_risc_v_application_exception_handler
.weak freertos_risc_v_application_exception_handler
freertos_risc_v_application_exception_handler:
    // Default exception handler - loop forever.
    j freertos_risc_v_application_exception_handler

.global freertos_risc_v_application_interrupt_handler
.weak freertos_risc_v_application_interrupt_handler
freertos_risc_v_application_interrupt_handler:
    // Default interrupt handler - loop forever.
    j freertos_risc_v_application_interrupt_handler
