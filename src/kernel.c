#include "gdt.h"
#include "stack.h"

/* ABI cdecl conservee : EAX (magic) puis EBX (informations Multiboot). */
void kernel_main(unsigned int magic, unsigned int boot_info)
{
    (void)magic;
    (void)boot_info;
    gdt_test();
    /* Les interruptions sont deja desactivees par _start. */
    stack_dump();
    for (;;)
        __asm__ volatile ("hlt" : : : "memory");
}
