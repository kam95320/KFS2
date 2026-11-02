#ifndef VGA_H
#define VGA_H

#define VGA_WHITE         0x0F
#define VGA_LIGHT_RED     0x0C
#define VGA_LIGHT_GREEN   0x0A
#define VGA_LIGHT_CYAN    0x0B
#define VGA_YELLOW        0x0E
#define VGA_LIGHT_MAGENTA 0x0D
#define VGA_LIGHT_BLUE    0x09

void vga_put_text_color(const char *text, unsigned int index,
    unsigned char color);
void vga_put_hex_color(unsigned int value, unsigned int index,
    unsigned char color);
void vga_print_hex_color(const char *label, unsigned int value,
    unsigned int row, unsigned char color);
void vga_clear(void);

#endif
