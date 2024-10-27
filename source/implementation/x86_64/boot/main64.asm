global longmode_start
extern kernel_main

section .text
bits 64

longmode_start:
    ; Load null into all data segment registers (setting them to flat mode)
    mov ax, 0
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
   
    ; Call kernel_main (64-bit C kernel function)
    call kernel_main

    ; Halt the CPU after kernel_main returns (shouldn't happen normally)
    hlt
