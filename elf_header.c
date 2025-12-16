#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <elf.h>
#include "util.h"
#include "elf_header.h"


/* Print ELF header */
void elf_header(FILE* f){

    rewind(f);
    Elf32_Ehdr hdr;
    fread(&hdr, 1, sizeof(Elf32_Ehdr), f);

    printf("En-tête ELF:\n");
    printf("Magique:   ");
    for( int i = 0; i < EI_NIDENT; i++){
        printf("%02x ", hdr.e_ident[i]);
    }
    printf("\n");

    printf("Classe:   %s\n", (hdr.e_ident[EI_CLASS] == ELFCLASS32 ? "ELF32" : (hdr.e_ident[EI_CLASS] == ELFCLASS64 ? "ELF64" : "Inconnu")));
    printf("Données:   %s\n", (hdr.e_ident[EI_DATA] == ELFDATA2LSB ? "Little Endian" : (hdr.e_ident[EI_DATA] == ELFDATA2MSB ? "Big Endian" : "Inconnu")));
    printf("Version:   %d\n", hdr.e_ident[EI_VERSION]);
    printf("OS/ABI:   %d\n", hdr.e_ident[EI_OSABI]);
    printf("Version ABI:   %d\n", hdr.e_ident[EI_ABIVERSION]);

    printf("Type:   0x%04x %s\n", reverse_2(hdr.e_type), (reverse_2(hdr.e_type) ==  ET_REL ? "REL (Fichier de réadressage)" : "Autres"));
    printf("Machine:   0x%04x %s\n", reverse_2(hdr.e_machine), (reverse_2(hdr.e_machine) == EM_ARM ? "ARM" : "Autres"));
    printf("Version:   0x%08x\n", reverse_4(hdr.e_version));
    printf("Adresse du point d'entrée:   0x%08x\n", reverse_4(hdr.e_entry));

    printf("Début des en-têtes de programme:   %u %s\n", reverse_4(hdr.e_phoff), "(octets dans le fichier)");
    printf("Début des en-têtes de section:   %u %s\n", reverse_4(hdr.e_shoff), "(octets dans le fichier)");

    printf("Flags:   0x%08x\n", reverse_4(hdr.e_flags));

    printf("Taille de cet en-tête: %u %s\n", reverse_2(hdr.e_ehsize), "(octets)");
    printf("Taille de l'en-tête du programme:   %u %s\n", reverse_2(hdr.e_phentsize), "(octets)");
    printf("Nombre d'en-tête du programme:   %u\n", reverse_2(hdr.e_phnum));
    printf("Taille des en-têtes de section:   %u %s\n", reverse_2(hdr.e_shentsize), "(octets)");
    printf("Nombre d'en-tête de section:   %u\n", reverse_2(hdr.e_shnum));
    printf("Table d'index des chaînes d'en-tête de section:   %u\n", reverse_2(hdr.e_shstrndx));

}

int check_spec(FILE* f){
    rewind(f);
    Elf32_Ehdr hdr;
    int be;
    uint16_t arch;
    fread(&hdr, 1, sizeof(Elf32_Ehdr), f);
    if( hdr.e_ident[EI_MAG0] != ELFMAG0 ||
        hdr.e_ident[EI_MAG1] != ELFMAG1 ||
        hdr.e_ident[EI_MAG2] != ELFMAG2 ||
        hdr.e_ident[EI_MAG3] != ELFMAG3 ){
        fprintf(stderr, "Not an ELF File.\n");
        return 0;
    }
    /* Does not allow ELF64, you can disable this condition, the program will works. */
    if (hdr.e_ident[EI_CLASS] != ELFCLASS32) {
        fprintf(stderr, "Not an ELF32 File\n");
        return 0;
    }

    be = (hdr.e_ident[EI_DATA] == ELFDATA2MSB);
    arch = be ? reverse_2(hdr.e_machine) : hdr.e_machine;

    if (arch != EM_ARM){
        fprintf(stderr, "Not a ARM Arch.\n");
        return 0;
    }

    if (hdr.e_ident[EI_DATA] != ELFDATA2MSB) {
    fprintf(stderr, "ELF non big endian\n");
    return 0;
    }
    return 1;
}

