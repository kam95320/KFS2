# KFS2 — GDT & Stack

Projet **Kernel from Scratch 2** de l’École 42 : noyau freestanding pour
**i386 (x86 32 bits)**, écrit en C et en assembleur NASM, démarré par GRUB.
Le sujet officiel version 2 est disponible dans [kfs2.pdf](kfs2.pdf).

## Fonctionnalités

- GDT installée à l’adresse physique `0x00000800` : descripteur nul et six
  segments (code, données et pile pour le noyau et l’utilisateur).
- Chargement de GDTR, saut lointain et rechargement des registres de segment.
- Pile noyau de 16 Kio, alignée sur 16 octets à sa réservation.
- Affichage VGA des bornes de pile, d’ESP et de cinq mots de 32 bits.
- Vérification de l’alignement et des bornes avant la lecture de 20 octets ;
  capture dans un tampon stable avant tout appel d’affichage.

Le noyau affiche le diagnostic de pile puis attend avec les interruptions
désactivées. Le diagnostic GDT exécuté auparavant est effacé par cet affichage.
Le shell proposé en bonus n’est pas implémenté.

## Prérequis

Les outils suivants doivent être disponibles dans le `PATH` :

- GCC avec prise en charge de `-m32` et de la cible i386 ;
- NASM et GNU binutils (`ld`, `nm`) ;
- GNU Make ;
- GRUB (`grub-mkrescue`, `grub-file`) avec ses modules pour BIOS PC ;
- xorriso pour créer l’ISO ;
- QEMU (`qemu-system-i386`) pour démarrer et tester le noyau ;
- Python 3 pour le test automatisé.

## Compiler et démarrer

Depuis la racine du dépôt :

```sh
make
make run
```

La compilation produit `build/kernel.elf` et `build/kfs2.iso` en utilisant
le script de liaison personnalisé `linker.ld`, sans bibliothèque hôte.

Pour démarrer directement l’image de livraison suivie dans Git :

```sh
qemu-system-i386 -cdrom images/kfs2.iso
```

L’ISO livrée mesure **5 287 936 octets**, soit moins de 10 MB.

## Vérifications

```sh
make check
```

Cette commande construit l’ISO si nécessaire, vérifie l’en-tête Multiboot,
l’absence de symboles non résolus et la taille de l’image, puis démarre QEMU.
Le test inspecte la mémoire VGA, GDTR, les sélecteurs CS/DS/SS, les sept
descripteurs et la correspondance entre les cinq mots capturés et affichés.
Les traces sont écrites dans `build/checks/`.

Pour reconstruire intégralement puis tester :

```sh
make re
make check
```

## Organisation

| Chemin | Rôle |
| --- | --- |
| `src/boot.asm` | En-tête Multiboot, installation GDT, réservation de pile et entrée assembleur |
| `src/kernel.c` | Enchaînement des diagnostics et attente finale |
| `src/gdt.c`, `src/gdt.h` | Lecture et affichage de GDTR et des sélecteurs |
| `src/stack.c`, `src/stack.h` | Capture protégée et affichage de la pile |
| `src/vga.c`, `src/vga.h` | Affichage texte et hexadécimal en mémoire VGA |
| `linker.ld` | Placement des sections du noyau à partir de `0x00100000` |
| `boot/grub/grub.cfg` | Démarrage automatique par GRUB |
| `tests/check_boot.py` | Contrôle automatisé du démarrage sous QEMU |
| `images/kfs2.iso` | Image de livraison démarrable, suivie dans Git |
| `archive/kfs3/` | Sources expérimentales antérieures, exclues de la construction KFS2 |
| `REPORT.md` | Rapport historique de refactorisation |

## Nettoyage et image de livraison

`make clean` supprime `build/` et l’ancien dossier `build-lab/`.
`make fclean` retire également les anciens artefacts `kernel.elf` et `kfs1.iso`
à la racine. Ces commandes conservent `images/kfs2.iso`.

La construction ne remplace pas automatiquement l’image livrée. Après une
modification du noyau, reconstruire et valider avant de la mettre à jour :

```sh
make re
make check
cp build/kfs2.iso images/kfs2.iso
```

Les fichiers temporaires de compilation restent ignorés par Git. Seule
`images/kfs2.iso` bénéficie d’une exception pour satisfaire la livraison de
l’image demandée au chapitre VII du sujet.
