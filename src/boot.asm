BITS 32

MULTIBOOT_MAGIC    equ 0x1BADB002
MULTIBOOT_FLAGS    equ 0
MULTIBOOT_CHECKSUM equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

section .multiboot progbits alloc noexec nowrite align=4
align 4
    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

section .text
global _start
extern kernel_main
; ================================================================
; Installation de la GDT a l'adresse physique 0x00000800
; Mode protege x86 32 bits - NASM
; ================================================================

gdt_install:
    pushad

    ; Copier 7 descripteurs de 8 octets vers 0x800
    mov esi, gdt_template
    mov edi, 0x00000800
    mov ecx, 14
    rep movsd

    ; Charger GDTR
    lgdt [gdt_pointer]

    ; Recharger CS avec un saut lointain
    jmp 0x08:.reload_cs

.reload_cs:
    ; Recharger les segments de donnees
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Segment de pile noyau
    mov ax, 0x18
    mov ss, ax

    popad
    ret

align 8
gdt_template:
    dq 0x0000000000000000   ; 0x00 : descripteur nul
    dq 0x00CF9A000000FFFF   ; 0x08 : code noyau
    dq 0x00CF92000000FFFF   ; 0x10 : donnees noyau
    dq 0x00CF92000000FFFF   ; 0x18 : pile noyau
    dq 0x00CFFA000000FFFF   ; 0x20 : code utilisateur
    dq 0x00CFF2000000FFFF   ; 0x28 : donnees utilisateur
    dq 0x00CFF2000000FFFF   ; 0x30 : pile utilisateur

gdt_pointer:
    dw 7 * 8 - 1
    dd 0x00000800

_start:
    cli
    cld

    mov esp, stack_top

    ; Installer notre GDT avant l'initialisation du noyau C
    call gdt_install

    sub esp, 8
    push ebx
    push eax
    call kernel_main
    add esp, 16

.halt:
    cli
    hlt
    jmp .halt

; ================================================================
; Pile du noyau : 16 Kio
; ================================================================

global stack_bottom
global stack_top

section .bss
align 16

stack_bottom:
    resb 16384
stack_top:

section .note.GNU-stack noalloc noexec nowrite progbits
