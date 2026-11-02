#include "vga.h"

/* Sans pagination : adresse physique du tampon texte VGA. */
static void vga_put_char_color(char character, unsigned int index,
    unsigned char color)
{
    volatile unsigned char *vga;
    unsigned int offset;

    vga = (volatile unsigned char *)0x000B8000;
    offset = index * 2;
    vga[offset] = (unsigned char)character;
    vga[offset + 1] = color;
}

void vga_put_text_color(const char *text, unsigned int index,
    unsigned char color)
{
    unsigned int i;

    i = 0;
    while (text[i])
    {
        vga_put_char_color(text[i], index + i, color);
        i++;
    }
}

void vga_put_hex_color(unsigned int value, unsigned int index,
    unsigned char color)
{
    const char *digits = "0123456789ABCDEF";
    unsigned int i;
    unsigned int shift;

    i = 0;
    while (i < 8)
    {
        shift = (7 - i) * 4;
        vga_put_char_color(digits[(value >> shift) & 0xF],
            index + i, color);
        i++;
    }
}

void vga_print_hex_color(const char *label,
    unsigned int value, unsigned int row, unsigned char color)
{
    unsigned int index;
    unsigned int i;

    index = row * 80;
    i = 0;

    vga_put_text_color(label, index, color);

    while (label[i])
        i++;

    vga_put_hex_color(value, index + i, color);
}

void vga_clear(void)
{
    unsigned int i;

    for (i = 0; i < 80 * 25; i++)
        vga_put_char_color(' ', i, VGA_WHITE);
}

