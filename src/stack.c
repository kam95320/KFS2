#include "stack.h"
#include "vga.h"

/* Symboles de la reservation assembleur de 16 Kio. */
extern unsigned char stack_bottom;
extern unsigned char stack_top;

void stack_dump(void)
{
    unsigned int esp;
    unsigned int bottom;
    unsigned int top;
    const volatile unsigned int *pointer;
    static unsigned int snapshot[5];
    unsigned int valid;
    unsigned int i;
    unsigned int row;

    /* Recuperer les adresses de la pile */
    __asm__ volatile ("mov %%esp, %0" : "=r"(esp) : : "memory");
    bottom = (unsigned int)&stack_bottom;
    top = (unsigned int)&stack_top;

    /* Capturer les cinq mots avant tout appel d'affichage. */
    valid = esp >= bottom && esp <= top &&
        (esp & 3u) == 0 && top - esp >= 5u * sizeof(unsigned int);
    if (valid)
    {
        pointer = (const volatile unsigned int *)esp;
        for (i = 0; i < 5; i++)
            snapshot[i] = pointer[i];
    }

    /* Effacer l'ecran VGA */
    vga_clear();

    vga_put_text_color("KERNEL STACK DUMP",
        0, VGA_YELLOW);

    /* Afficher les limites et ESP */
    vga_print_hex_color("STACK BOTTOM: ",
        bottom, 1, VGA_LIGHT_CYAN);

    vga_print_hex_color("STACK TOP: ",
        top, 2, VGA_LIGHT_CYAN);

    vga_print_hex_color("ESP SNAPSHOT: ",
        esp, 3, VGA_LIGHT_CYAN);

    /* Verifier les limites avant de lire cinq mots */
    if (!valid)
    {
        vga_put_text_color("STACK DUMP: OUT OF BOUNDS",
            5 * 80, VGA_LIGHT_RED);
        return;
    }

    vga_put_text_color("ADDRESS       VALUE",
        5 * 80, VGA_WHITE);

    i = 0;

    while (i < 5)
    {
        row = 6 + i;

        vga_put_hex_color(
            esp + i * sizeof(unsigned int),
            row * 80,
            VGA_LIGHT_CYAN);

        vga_put_hex_color(
            snapshot[i],
            row * 80 + 15,
            VGA_LIGHT_GREEN);

        i++;
    }
}


