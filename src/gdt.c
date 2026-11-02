#include "gdt.h"
#include "vga.h"

void gdt_test(void)
{
    struct
    {
        unsigned short limit;
        unsigned int base;
    } __attribute__((packed)) gdtr;

    unsigned short cs;
    unsigned short ds;
    unsigned short ss;

    __asm__ volatile ("sgdt %0" : "=m"(gdtr));
    __asm__ volatile ("mov %%cs, %0" : "=r"(cs));
    __asm__ volatile ("mov %%ds, %0" : "=r"(ds));
    __asm__ volatile ("mov %%ss, %0" : "=r"(ss));

    vga_print_hex_color("GDT BASE: ",
        gdtr.base, 15, VGA_LIGHT_GREEN);

    vga_print_hex_color("GDT LIMIT: ",
        gdtr.limit, 16, VGA_LIGHT_GREEN);

    vga_print_hex_color("GDT CS SELECTOR: ",
        cs, 17, VGA_LIGHT_GREEN);

    vga_print_hex_color("GDT DS SELECTOR: ",
        ds, 18, VGA_LIGHT_GREEN);

    vga_print_hex_color("GDT SS SELECTOR: ",
        ss, 19, VGA_LIGHT_GREEN);
}


