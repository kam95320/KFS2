# Refactorisation KFS2

## Retour arriere

Le repertoire .git present est vide : aucun depot Git exploitable. Avant les
modifications, copie de src/, boot/, Makefile, Makefile.avant-build-lab,
linker.ld, kernel.elf et kfs1.iso dans /tmp/kfs2-before.
La copie durable des sources initiales se trouve dans archive/kfs3/.
Aucun commit ni push.

## Arborescence

Avant (hors repertoires proteges vides) :

```text
.gitignore
Makefile
Makefile.avant-build-lab
linker.ld
boot/grub/grub.cfg
src/{boot.asm,kernel.c}
build/                         # genere
build-lab/{boot.o,kernel.o,iso/} # genere
kernel.elf                     # genere
kfs1.iso                       # genere
```

Apres :

```text
.gitignore
Makefile
Makefile.avant-build-lab        # ancienne reference conservee
linker.ld                      # inchange
REPORT.md
boot/grub/grub.cfg
src/{boot.asm,kernel.c,vga.c,vga.h,gdt.c,gdt.h,stack.c,stack.h}
archive/kfs3/{README.md,Makefile,linker.ld,src/,boot/}
tests/check_boot.py
build/{boot.o,kernel.o,vga.o,gdt.o,stack.o,*.d,kernel.elf,kfs2.iso,iso/,checks*/}
```

## Classement exhaustif et mouvements

| Categorie | Fonctions / symboles de code initiaux | Destination |
|---|---|---|
| Obligatoire KFS2 | gdt_install ; stack_dump | boot.asm ; stack.c |
| Demarrage et support obligatoire | _start ; vga_put_char_color, vga_put_text_color, vga_put_hex_color, vga_print_hex_color, vga_clear ; kernel_main | boot.asm ; vga.c ; kernel.c |
| Diagnostic evaluation | gdt_test | gdt.c |
| Preparation KFS3 | paging_prepare, paging_enable ; alloc_page ; idt_set_gate, idt_prepare ; outb, pic_write, pic_prepare ; timer_handler, breakpoint_handler, page_fault_repair, page_fault_handler ; page_fault_entry, breakpoint_entry, timer_entry ; allocation_test ; heap_init, heap_split, kmalloc, heap_merge, kfree, heap_test | archive/kfs3/src/ : sources completes initiales |
| Experimental sans consommateur actuel | collecte mmap integree a kernel_main, memory_regions, memory_region_count | archive/kfs3/src/kernel.c |

Aucune fonction initiale n'est detruite definitivement. Les fonctions KFS3 et
leurs donnees sont retirees de la liaison KFS2. La collecte mmap n'avait aucun
lecteur : elle reste archivee. Aucun autre code n'a ete declare mort sur le seul
critere d'absence d'appel dans kernel_main.

References indirectes verifiees : _start est ENTRY dans linker.ld ; les stubs
page_fault_entry, breakpoint_entry et timer_entry appellent leurs gestionnaires
C externes et sont installes par adresse dans l'IDT. paging_enable est appele
depuis C. stack_bottom et stack_top sont definis en assembleur et references
par stack.c. Les labels reload_cs et halt sont internes a boot.asm.

## Responsabilites et dependances

boot.asm garde l'en-tete Multiboot, la GDT (nul + six segments), sa copie
physique a 0x800, LGDT, le saut lointain, les rechargements DS/ES/FS/GS/SS,
la reservation de 16 Kio et l'initialisation ESP. L'ABI cdecl et les deux
arguments Multiboot transmis a kernel_main restent identiques.

kernel_main appelle gdt_test, puis stack_dump et attend avec HLT, interruptions
desactivees depuis _start. gdt.c lit GDTR/CS/DS/SS et utilise vga.h.
stack.c utilise vga.h et les deux symboles assembleur de bornes. Le tampon
snapshot reste static ; le pilote VGA ne publie aucune variable globale et
son ecriture elementaire reste static. Les headers ne publient que les
interfaces et les couleurs utilisees. Aucun module additionnel de production.

L'ancien alias VGA 0x00400000 dependait de paging_prepare/paging_enable.
Sans pagination, vga.c utilise explicitement l'adresse physique 0xB8000.
IDT, PIC, exceptions de test, allocation, tas et mmap sortent du demarrage.
L'installation GDT precede toujours C ; le diagnostic GDT precede toujours
le stack dump. Comme avant, le stack dump efface le diagnostic GDT de l'ecran.

## Construction et fichiers modifies

Modifies : Makefile, boot/grub/grub.cfg, src/kernel.c, src/boot.asm.
Ajoutes : six fichiers vga/gdt/stack, tests/check_boot.py, REPORT.md,
archive/kfs3/. linker.ld reste identique.

Le Makefile compile explicitement les quatre modules C et boot.asm, suit les
headers avec -MMD -MP, et lie exclusivement ces objets avec ld elf_i386.
Options freestanding, absence de runtime hote, -march=i386, -Wall -Wextra
-Werror. Tous les produits courants sont sous build/. all, run, clean, fclean,
re restent disponibles ; check ajoute les controles statiques et un boot QEMU.
L'ancien nom kfs1.iso devient build/kfs2.iso. GRUB demarre automatiquement
(timeout=0) et affiche le nom KFS2. Les limites de 10 000 000 octets sont
controlees pour ELF et ISO (plus strict que 10 Mio).

## Validation

Les resultats mesures sont completes ci-dessous. Le test QEMU est un test
externe : il lit les registres, la RAM physique VGA, les sept descripteurs et
la capture static. Il verifie les cinq adresses successives, les valeurs
capturees affichees et leur appartenance aux bornes de pile. Le bit accessed
des descripteurs est autorise a etre positionne par le CPU.

La pile courante peut etre reutilisee par les appels VGA apres capture : comparer
les valeurs affichees a la RAM de la pile au repos serait incorrect. Ce test
compare donc au tampon capture. Il ne remplace pas un arret instruction par
instruction au moment de chaque lecture, ni une injection de tous les cas
invalides des precedents tests de robustesse (scripts non presents ici).

| Controle execute | Resultat |
|---|---|
| Reconstruction initiale dans /tmp/kfs2-before et boot de reference | Reussis |
| make re apres refactorisation | Reussi, aucun avertissement C/assembleur/liaison |
| Compilation stricte | -Wall -Wextra -Werror, quatre modules C |
| nm -u build/kernel.elf | Sortie vide : aucun symbole non resolu |
| grub-file --is-x86-multiboot | Code retour 0 |
| readelf -h -l | ELF32 Intel 80386, EXEC, pas de segment INTERP/DYNAMIC, GNU_STACK RW |
| En-tete Multiboot | Section chargeable a l'offset 0x1000, dans les 8192 premiers octets |
| QEMU via ISO GRUB | Trois demarrages independants reussis |
| GDTR et segments sur les trois boots | Base 0x800, limite 0x37, CS=0x08, DS=0x10, SS=0x18 |
| Sept descripteurs en RAM physique | Conformes au modele, avec bit accessed CPU autorise |
| Stack dump sur les trois boots | Cinq couples adresses/valeurs conformes a la capture ; adresses alignees dans la pile |
| Pagination retiree | CR0=0x00000011, CR3=0 dans la trace finale ; VGA physique operationnel |
| Taille ELF avant / apres | 15 100 / 13 704 octets |
| Taille ISO avant / apres | 5 289 984 / 5 287 936 octets ; inferieur a 10 MB |
| Integrite archive | cmp des deux sources avec la sauvegarde : identiques ; linker.ld inchange |

Exemple mesure apres (identique sur les trois boots) :

```text
STACK BOTTOM: 00103000
STACK TOP: 00107000
ESP SNAPSHOT: 00106FB0
ADDRESS       VALUE
00106FB0       0010204C
00106FB4       00000018
00106FB8       00106FB0
00106FBC       00000001
00106FC0       00107000
```

Reference reconstruite : memes bornes, ESP=0x00106F98 et valeurs
00000080, 00000000, 00106F98, 00000001, 00107000. Les adresses et valeurs
internes de pile ne sont pas des constantes contractuelles : simplification de
kernel_main, changement d'adresses des fonctions et retrait des appels KFS3
modifient normalement son contenu. Les cinq pas de quatre octets et les bornes
restent valides. L'affichage final garde la meme presentation.

Traces consultables : build/compilation.log, build/reference-before/,
build/checks/, build/checks-2/, build/checks-3/ (screen.txt, registers.txt,
vga.bin, snapshot.bin, gdt.bin, monitor.txt, qemu.log). Les traces brutes du
moniteur contiennent ses sequences d'edition de ligne. make check reproduit
les controles statiques et un boot ; relancer deux fois pour trois boots.

## Regressions et limites

Aucune regression observee sur les exigences KFS2 enumerees dans la demande.
Les tests KFS3 et la collecte mmap sont volontairement absents du noyau livre,
mais integralement conserves dans archive/kfs3/. Le diagnostic GDT est encore
efface par le stack dump, comme dans la reference ; les registres sont verifies
independamment par QEMU. Le choix timeout=0 rend le boot GRUB automatique.
Les noms/emplacements des artefacts changent intentionnellement ; utiliser
make run ou build/kfs2.iso.

Les anciens scripts/tests de robustesse mentionnes dans la demande ne sont pas
presents. Leur couverture complete n'est donc pas revendiquee. La logique du
stack dump et ses gardes sont conservees ; les tests executes ici couvrent les
boots normaux et la coherence du resultat capture/affiche. Validation sous
QEMU uniquement, sans execution sur machine physique. Le sujet officiel
n'etait pas fourni : la conclusion de conformite porte sur les exigences
explicitement enumerees par l'utilisateur.

Le resultat constitue un noyau KFS2 autonome, freestanding i386, demarrant via
GRUB, avec GDT a l'adresse physique imposee, pile reservee/initialisee et stack
dump securise conserve. Aucune fonctionnalite nouvelle du noyau n'a ete ajoutee.
