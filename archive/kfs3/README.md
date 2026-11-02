# Copie de travail KFS3 avant nettoyage

Copie integrale des deux sources, du Makefile, de linker.ld et de la configuration
GRUB avant refactorisation. Aucune logique experimentale n'a ete modifiee.
Ce repertoire n'est pas compile par le Makefile KFS2.

Il conserve la pagination et son alias VGA 0x00400000 -> 0xB8000, l'IDT,
les entrees assembleur d'exceptions/IRQ, le PIC, le reparateur de page fault,
le pool de pages, le tas et ses tests, ainsi que la collecte Multiboot mmap.
Les gestionnaires C sont references par les stubs assembleur ; les stubs sont
references par idt_prepare. Ils ne sont donc pas du code mort.

Pour reproduire l'ancien noyau sans produire de fichiers ici : copier ce dossier
vers /tmp/kfs3-work puis executer make dans cette copie (outils i386 requis).
Cette conservation ne constitue pas une validation de robustesse pour KFS3.
