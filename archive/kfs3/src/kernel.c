
/* ================================================================ */
/* Affichage VGA                                                    */
/* ================================================================ */

#define VGA_WHITE         0x0F
#define VGA_LIGHT_RED     0x0C
#define VGA_LIGHT_GREEN   0x0A
#define VGA_LIGHT_CYAN    0x0B
#define VGA_YELLOW        0x0E
#define VGA_LIGHT_MAGENTA 0x0D
#define VGA_LIGHT_BLUE    0x09

static void vga_put_char_color(char character, unsigned int index,
    unsigned char color)
{
    volatile unsigned char *vga;
    unsigned int offset;

    vga = (volatile unsigned char *)0x00400000;
    offset = index * 2;
    vga[offset] = (unsigned char)character;
    vga[offset + 1] = color;
}

static void vga_put_text_color(const char *text, unsigned int index,
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

static void vga_put_hex_color(unsigned int value, unsigned int index,
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

static void vga_print_hex_color(const char *label,
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

static void vga_clear(void)
{
    unsigned int i;

    for (i = 0; i < 80 * 25; i++)
        vga_put_char_color(' ', i, VGA_WHITE);
}

/* ================================================================ */
/* Pagination : pages de 4 096 octets                               */
/* ================================================================ */

static unsigned int page_directory[1024]
    __attribute__((aligned(4096)));

static unsigned int page_table[1024]
    __attribute__((aligned(4096)));

static unsigned int recovery_table[1024]
    __attribute__((aligned(4096)));

static unsigned char recovery_page[4096]
    __attribute__((aligned(4096)));

static unsigned int allocation_table[1024]
    __attribute__((aligned(4096)));

extern void paging_enable(unsigned int directory_address);

static unsigned int vga_page_table[1024]
    __attribute__((aligned(4096)));

/* Limites de la pile definies dans boot.asm */
extern unsigned char stack_bottom;
extern unsigned char stack_top;


static void paging_prepare(void)
{
    unsigned int i;

    i = 0;

    while (i < 1024)
    {
        page_directory[i] = 0;

        /* Mappage identitaire : virtuel = physique */
        page_table[i] = (i * 4096u) | 0x003;

        /* Table reservee a la memoire VGA */
        vga_page_table[i] = 0;

        i++;
    }

    /* 0x00000000 - 0x003FFFFF : mappage identitaire */
    page_directory[0] = (unsigned int)page_table | 0x003;

    /* 0x00400000 : memoire VGA physique 0x000B8000 */
    vga_page_table[0] = 0x000B8000 | 0x003;

    page_directory[1] = (unsigned int)vga_page_table | 0x003;
}

/* ================================================================ */
/* Allocation de pages dans une reserve de huit pages               */
/* ================================================================ */

static unsigned char page_pool[8][4096]
    __attribute__((aligned(4096)));

static unsigned char pool_bitmap;

static unsigned int alloc_page(void)
{
    unsigned int numero;
    unsigned int masque;

    numero = 0;
    while (numero < 8)
    {
        masque = 1u << numero;

        if ((pool_bitmap & masque) == 0)
        {
            pool_bitmap |= masque;
            return ((unsigned int)&page_pool[numero][0]);
        }

        numero++;
    }

    return (0);
}

/* ================================================================ */
/* IDT                                                              */
/* ================================================================ */

struct idt_entry
{
    unsigned short offset_low;
    unsigned short selector;
    unsigned char zero;
    unsigned char attributes;
    unsigned short offset_high;
} __attribute__((packed));

struct idt_pointer
{
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

static struct idt_entry idt[256];

extern void page_fault_entry(void);
extern void breakpoint_entry(void);
extern void timer_entry(void);

static void idt_set_gate(unsigned int vector, unsigned int address,
    unsigned short selector)
{
    idt[vector].offset_low = address & 0xFFFF;
    idt[vector].selector = selector;
    idt[vector].zero = 0;
    idt[vector].attributes = 0x8E;
    idt[vector].offset_high = address >> 16;
}

static void idt_prepare(void)
{
    struct idt_pointer pointer;
    unsigned short code_selector;

    __asm__ volatile ("mov %%cs, %0" : "=r"(code_selector));

    idt_set_gate(14, (unsigned int)page_fault_entry, code_selector);
    idt_set_gate(3, (unsigned int)breakpoint_entry, code_selector);
    idt_set_gate(32, (unsigned int)timer_entry, code_selector);

    pointer.limit = sizeof(idt) - 1;
    pointer.base = (unsigned int)&idt[0];

    __asm__ volatile ("lidt %0" : : "m"(pointer) : "memory");
}

/* ================================================================ */
/* Ports d'entree-sortie et PIC                                     */
/* ================================================================ */

static void outb(unsigned short port, unsigned char value)
{
    __asm__ volatile ("outb %0, %1"
        :
        : "a"(value), "Nd"(port)
        : "memory");
}

static void pic_write(unsigned short port, unsigned char value)
{
    outb(port, value);
    outb(0x80, 0);
}

static void pic_prepare(void)
{
    pic_write(0x20, 0x11);
    pic_write(0xA0, 0x11);
    pic_write(0x21, 0x20);
    pic_write(0xA1, 0x28);
    pic_write(0x21, 0x04);
    pic_write(0xA1, 0x02);
    pic_write(0x21, 0x01);
    pic_write(0xA1, 0x01);

    outb(0x21, 0xFE);
    outb(0xA1, 0xFF);
}

/* ================================================================ */
/* Gestionnaires d'exceptions et d'interruptions                    */
/* ================================================================ */

static unsigned int timer_ticks;

void timer_handler(void)
{
    timer_ticks++;

    vga_print_hex_color("TIMER INTERRUPT COUNT: ",
        timer_ticks, 6, VGA_YELLOW);

    outb(0x20, 0x20);
}

void breakpoint_handler(void)
{
    vga_put_text_color("BREAKPOINT HANDLER: OK",
        4 * 80, VGA_LIGHT_CYAN);
}

static void page_fault_repair(void)
{
    recovery_table[0] = (unsigned int)recovery_page | 0x003;
    page_directory[2] = (unsigned int)recovery_table | 0x003;

    __asm__ volatile ("invlpg (%0)"
        :
        : "r"(0x00800000)
        : "memory");
}

void page_fault_handler(unsigned int error_code)
{
    unsigned int address;

    __asm__ volatile ("mov %%cr2, %0" : "=r"(address));

    vga_print_hex_color("PAGE FAULT ADDRESS: ",
        address, 1, VGA_LIGHT_RED);

    vga_print_hex_color("PAGE FAULT ERROR CODE: ",
        error_code, 2, VGA_LIGHT_RED);

    if (address == 0x00800000 && error_code == 0x00000002)
    {
        page_fault_repair();
        return;
    }

    for (;;)
        __asm__ volatile ("cli; hlt");
}

/* ================================================================ */
/* Reservation et mappage de la page utilisee par le tas             */
/* ================================================================ */

static void allocation_test(void)
{
    unsigned int physical;
    volatile unsigned int *virtual;

    physical = alloc_page();

    if (physical == 0)
    {
        vga_put_text_color("PAGE ALLOCATION: FAILED",
            7 * 80, VGA_LIGHT_RED);

        for (;;)
            __asm__ volatile ("cli; hlt");
    }

    allocation_table[0] = physical | 0x003;
    page_directory[3] = (unsigned int)allocation_table | 0x003;

    virtual = (volatile unsigned int *)0x00C00000;

    __asm__ volatile ("invlpg (%0)"
        : : "r"(virtual) : "memory");

    *virtual = 42;

    vga_print_hex_color("ALLOCATED PHYSICAL PAGE: ",
        physical, 7, VGA_LIGHT_MAGENTA);

    vga_print_hex_color("VIRTUAL PAGE READ VALUE: ",
        *virtual, 8, VGA_LIGHT_MAGENTA);
}

/* ================================================================ */
/* Tas : en-tetes de 16 octets en mode 32 bits                       */
/* ================================================================ */

struct heap_block
{
    unsigned int size;
    unsigned int free;
    struct heap_block *next;
    unsigned int padding;
};

static struct heap_block *heap_first;

static void heap_init(void)
{
    heap_first = (struct heap_block *)0x00C00000u;
    heap_first->size = 4096u - sizeof(struct heap_block);
    heap_first->free = 1;
    heap_first->next = 0;
    heap_first->padding = 0;
}

/*
 * Conditions d'appel :
 * - block est libre ;
 * - size est un multiple de 16 ;
 * - size ne depasse pas block->size.
 */
static void heap_split(struct heap_block *block,
    unsigned int size)
{
    struct heap_block *rest;

    if (block->size - size < sizeof(struct heap_block) + 16u)
        return;

    rest = (struct heap_block *)((unsigned char *)(block + 1)
            + size);

    rest->size = block->size - size - sizeof(struct heap_block);
    rest->free = 1;
    rest->next = block->next;
    rest->padding = 0;

    block->size = size;
    block->next = rest;
}

static void *kmalloc(unsigned int size)
{
    struct heap_block *block;

    if (size == 0 || size > 4096u - sizeof(struct heap_block))
        return (0);

    size = (size + 15u) & ~15u;
    block = heap_first;

    while (block)
    {
        if (block->free && block->size >= size)
        {
            heap_split(block, size);
            block->free = 0;
            return ((void *)(block + 1));
        }

        block = block->next;
    }

    return (0);
}

/*
 * Les blocs de cette liste sont contigus et classes par adresse.
 * Deux blocs voisins libres peuvent donc etre fusionnes.
 */
static void heap_merge(void)
{
    struct heap_block *block;
    struct heap_block *next;

    block = heap_first;

    while (block && block->next)
    {
        next = block->next;

        if (block->free && next->free)
        {
            block->size += sizeof(struct heap_block) + next->size;
            block->next = next->next;
        }
        else
            block = block->next;
    }
}

/*
 * pointer doit etre nul ou designer un bloc encore alloue,
 * precedemment renvoye par kmalloc.
 */
static void kfree(void *pointer)
{
    struct heap_block *block;

    if (pointer == 0)
        return;

    block = (struct heap_block *)pointer - 1;
    block->free = 1;

    heap_merge();
}

/* ================================================================ */
/* Test : liberation de A, puis reutilisation par C                 */
/* ================================================================ */

static void heap_test(void)
{
    char *a;
    char *b;
    char *c;
    unsigned int old_address;
    unsigned int reuse_ok;

    a = kmalloc(100);
    b = kmalloc(200);

    if (!a || !b)
    {
        vga_put_text_color("HEAP ALLOCATION: FAILED",
            9 * 80, VGA_LIGHT_RED);
        return;
    }

    old_address = (unsigned int)a;
    b[0] = 'B';

    kfree(a);
    c = kmalloc(100);

    if (!c)
    {
        vga_put_text_color("HEAP REUSE: FAILED",
            9 * 80, VGA_LIGHT_RED);
        return;
    }

    c[0] = 'C';

    vga_print_hex_color("HEAP OLD BLOCK ADDRESS: ",
        old_address, 9, VGA_LIGHT_MAGENTA);

    vga_print_hex_color("HEAP REUSED BLOCK ADDRESS: ",
        (unsigned int)c, 10, VGA_LIGHT_MAGENTA);

    reuse_ok = (old_address == (unsigned int)c
        && c[0] == 'C' && b[0] == 'B');

    if (reuse_ok)
        vga_put_text_color("HEAP REUSE AND DATA: OK",
            11 * 80, VGA_LIGHT_GREEN);
    else
        vga_put_text_color("HEAP REUSE AND DATA: FAILED",
            11 * 80, VGA_LIGHT_RED);

    kfree(c);
    kfree(b);

    vga_print_hex_color("HEAP MERGED FREE SIZE: ",
        heap_first->size, 12, VGA_LIGHT_MAGENTA);

    vga_print_hex_color("HEAP FIRST BLOCK FREE FLAG: ",
        heap_first->free, 13, VGA_LIGHT_MAGENTA);

    vga_print_hex_color("HEAP NEXT BLOCK ADDRESS: ",
        (unsigned int)heap_first->next, 14,
        VGA_LIGHT_MAGENTA);
}



struct multiboot_mmap_entry
{
    unsigned int size;
    unsigned long long addr;
    unsigned long long len;
    unsigned int type;
} __attribute__((packed));

struct memory_region
{
    unsigned int base_low;
    unsigned int length_low;
    unsigned int type;
};

static struct memory_region memory_regions[6];
static unsigned int memory_region_count;

static void gdt_test(void)
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


static void stack_dump(void)
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


/* ================================================================ */
/* Point d'entree C du noyau                                        */
/* ================================================================ */


void kernel_main(unsigned int magic, unsigned int boot_info)
{
    volatile unsigned int *unmapped;
    unsigned int flags;
    unsigned int mmap_length;
    unsigned int mmap_addr;
    unsigned int offset;
    unsigned int entry_size;
    const unsigned int *mbi;
    const struct multiboot_mmap_entry *entry;

    /* ============================================================ */
    /* 1. Recuperation des informations Multiboot                    */
    /* ============================================================ */

    flags = 0;
    mmap_length = 0;
    mmap_addr = 0;

    if (magic == 0x2BADB002 && boot_info != 0)
    {
        mbi = (const unsigned int *)boot_info;
        flags = mbi[0];

        if (flags & (1u << 6))
        {
            mmap_length = mbi[11];
            mmap_addr = mbi[12];
        }
    }

    /* ============================================================ */
    /* 2. Lecture de la carte memoire avant la pagination            */
    /* ============================================================ */

    memory_region_count = 0;

    if ((flags & (1u << 6)) && mmap_addr != 0)
    {
        offset = 0;

        while (offset < mmap_length && memory_region_count < 6)
        {
            if (mmap_length - offset < 4)
                break;

            entry = (const struct multiboot_mmap_entry *)
                (mmap_addr + offset);

            entry_size = entry->size + 4;

            if (entry->size < 20 ||
                entry_size > mmap_length - offset)
                break;

            memory_regions[memory_region_count].base_low =
                (unsigned int)entry->addr;

            memory_regions[memory_region_count].length_low =
                (unsigned int)entry->len;

            memory_regions[memory_region_count].type =
                entry->type;

            memory_region_count++;
            offset += entry_size;
        }
    }

    /* ============================================================ */
    /* 3. Initialisation de la pagination et de l'IDT                */
    /* ============================================================ */

    paging_prepare();
    idt_prepare();
    paging_enable((unsigned int)page_directory);

    /* ============================================================ */
    /* 4. Test de l'affichage VGA                                    */
    /* ============================================================ */

    vga_put_text_color("VGA OUTPUT TEST: 42",
        0, VGA_WHITE);

    /* ============================================================ */
    /* 5. Test du defaut de page et de sa reparation                 */
    /* ============================================================ */

    unmapped = (volatile unsigned int *)0x00800000;
    *unmapped = 42;

    vga_print_hex_color("PAGE FAULT RECOVERY VALUE: ",
        *unmapped, 3, VGA_LIGHT_GREEN);

    /* ============================================================ */
    /* 6. Test du breakpoint INT3                                    */
    /* ============================================================ */

    __asm__ volatile ("int3" : : : "memory");

    vga_put_text_color("BREAKPOINT RETURN: OK",
        5 * 80, VGA_LIGHT_CYAN);

    /* ============================================================ */
    /* 7. Test de l'allocation physique et virtuelle                 */
    /* ============================================================ */

    allocation_test();

    /* ============================================================ */
    /* 8. Initialisation et test du tas du noyau                     */
    /* ============================================================ */

    heap_init();
    heap_test();

    /* ============================================================ */
    /* 9. Verification de la GDT                                     */
    /* ============================================================ */

    gdt_test();

    /* ============================================================ */
    /* 10. Initialisation du PIC                                     */
    /* ============================================================ */

    pic_prepare();

    /* ============================================================ */
    /* 11. Diagnostic final : pile du noyau                          */
    /* ============================================================ */

    /*
     * Les interruptions restent desactivees pour eviter
     * que le timer modifie l'affichage du stack dump.
     */
    __asm__ volatile ("cli" : : : "memory");

    stack_dump();

    /* ============================================================ */
    /* 12. Arret du noyau pour observer le diagnostic                */
    /* ============================================================ */

    for (;;)
        __asm__ volatile ("hlt" : : : "memory");
}
