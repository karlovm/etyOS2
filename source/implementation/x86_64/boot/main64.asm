global longmode_start
extern kernel_main

; enter 64-bit mode
section .text
bits 64
longmode_start:
    ; load null into all data segment registers
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

	call kernel_main
    hlt