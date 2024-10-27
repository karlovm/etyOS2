global start
extern longmode_start

section .data
; Error messages
ERR_PREFIX:    db 'etyOS2 Kernel Load Error: ', 0
MULTIBOOT_ERR: db 'Multiboot check failed', 0
CPUID_ERR:     db 'CPUID not supported', 0
LONGMODE_ERR:  db 'Long mode not supported', 0

section .text
bits 32
start:
    mov esp, stack_top

    call check_multiboot
    call check_cpuid
    call check_longmode

    call setup_page_tables
    call enable_paging

    lgdt [gdt64.pointer]
    jmp gdt64.code_segment:longmode_start

    hlt

check_multiboot:
    cmp eax, 0x36d76289
    jne .no_multiboot
    ret
.no_multiboot:
    mov al, 'M'          ; Store 'M' for Multiboot error
    jmp error

check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    mov al, 'C'          ; Store 'C' for CPUID error
    jmp error

check_longmode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_longmode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_longmode
    
    ret
.no_longmode:
    mov al, 'L'          ; Store 'L' for Long Mode error
    jmp error

setup_page_tables:
    mov eax, page_table_l3
    or eax, 0b11 ; present, writable
    mov [page_table_l4], eax
    
    mov eax, page_table_l2
    or eax, 0b11 ; present, writable
    mov [page_table_l3], eax

    mov ecx, 0 ; counter
.loop:
    mov eax, 0x200000 ; 2MiB
    mul ecx
    or eax, 0b10000011 ; present, writable, huge page
    mov [page_table_l2 + ecx * 8], eax

    inc ecx ; increment counter
    cmp ecx, 512 ; checks if the whole table is mapped
    jne .loop ; if not, continue

    ret

enable_paging:
    ; pass page table location to cpu
    mov eax, page_table_l4
    mov cr3, eax

    ; enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ret

error:
    push eax             ; Save error code
    call display_error
    jmp $               ; Infinite loop

display_error:
    ; Video memory address for text mode
    mov edi, 0xb8000
    
    ; Clear screen first (fill with spaces)
    push eax            ; Save error code during screen clear
    mov ecx, 80*25      ; screen size (80x25)
    mov ax, 0x4020      ; red background (0x40), space character (0x20)
    rep stosw           ; repeat store word
    pop eax             ; Restore error code
    
    ; Reset to start of video memory
    mov edi, 0xb8000
    
    ; Print "etyOS2 Kernel Load Error: " prefix
    mov esi, ERR_PREFIX
    call print_string
    
    ; Get the error code and determine which message to display
    mov esi, MULTIBOOT_ERR    ; Default to multiboot error
    cmp al, 'M'
    je .print_error_msg
    cmp al, 'C'
    jne .not_cpuid
    mov esi, CPUID_ERR
    jmp .print_error_msg
.not_cpuid:
    cmp al, 'L'
    jne .print_error_msg
    mov esi, LONGMODE_ERR

.print_error_msg:
    call print_string
    ret

; Helper function to print null-terminated string
; Input: esi = string address
print_string:
    push eax                  ; Save registers we'll modify
    push ecx
    mov ah, 0x4F             ; White text on red background
.loop:
    lodsb                    ; Load next character
    test al, al              ; Check for null terminator
    jz .done
    mov word [edi], ax       ; Store character and attribute
    add edi, 2               ; Move to next character position
    jmp .loop
.done:
    pop ecx                  ; Restore registers
    pop eax
    ret

section .bss
align 4096
page_table_l4:
    resb 4096
page_table_l3:
    resb 4096
page_table_l2:
    resb 4096
stack_bottom:
    resb 4096 * 4
stack_top:

section .rodata
gdt64:
    dq 0 ; zero entry
.code_segment: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; code segment
.pointer:
    dw $ - gdt64 - 1 ; length
    dq gdt64 ; address