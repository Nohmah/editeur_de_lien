#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <elf.h>
#include "util.h"
#include "elf_header.h"

ElfHeaderData header;

void afficher_entete() {

    printf("ELF Header:\n");
    printf("Magic:   ");
    for(int i = 0; i < EI_NIDENT; i++) {
        printf("%02x ", header.data.e_ident[i]);
    }
    printf("\n");

    printf("Class:   %s\n", (header.data.e_ident[EI_CLASS] == ELFCLASS32 ? "ELF32" : "Inconnu"));
    printf("Data:   %s\n", (header.data.e_ident[EI_DATA] == ELFDATA2MSB ? "Big Endian" : "Inconnu"));
    printf("Version:   %d\n", header.data.e_ident[EI_VERSION]);
    printf("OS/ABI:   %d\n", header.data.e_ident[EI_OSABI]);
    printf("ABI Version:   %d\n", header.data.e_ident[EI_ABIVERSION]);

    printf("Type:   %s\n", (header.data.e_type == ET_REL ? "REL (Fichier de réadressage)" : "Autres"));
    printf("Machine:   %s\n", (header.data.e_machine == EM_ARM ? "ARM" : "Inconnu"));
    printf("Version:   0x%08x\n", header.data.e_version);
    printf("Entry point address:   0x%08x\n", header.data.e_entry);

    printf("Start of program headers:   %u %s\n", header.data.e_phoff, "(octets dans le fichier)");
    printf("Start of section headers:   %u %s\n", header.data.e_shoff, "(octets dans le fichier)");

    printf("Flags:   0x%08x\n", header.data.e_flags);

    printf("Size of this header: %u %s\n", header.data.e_ehsize, "(octets)");
    printf("Size of program headers:   %u %s\n", header.data.e_phentsize, "(octets)");
    printf("Number of program headers:   %u\n", header.data.e_phnum);
    printf("Size of section headers:   %u %s\n", header.data.e_shentsize, "(octets)");
    printf("Number of section headers:   %u\n", header.data.e_shnum);
    printf("Section header string table index:   %u\n", header.data.e_shstrndx);
}

int verifier_fichier(FILE* f){
    rewind(f);

    fread(&header.data, 1, sizeof(Elf32_Ehdr), f);

    header.data.e_type = reverse_2(header.data.e_type);
    header.data.e_machine = reverse_2(header.data.e_machine);
    header.data.e_version = reverse_4(header.data.e_version);
    header.data.e_entry = reverse_4(header.data.e_entry);
    header.data.e_phoff = reverse_4(header.data.e_phoff);
    header.data.e_shoff = reverse_4(header.data.e_shoff);
    header.data.e_flags = reverse_4(header.data.e_flags);
    header.data.e_ehsize = reverse_2(header.data.e_ehsize);
    header.data.e_phentsize = reverse_2(header.data.e_phentsize);
    header.data.e_phnum = reverse_2(header.data.e_phnum);
    header.data.e_shentsize = reverse_2(header.data.e_shentsize);
    header.data.e_shnum = reverse_2(header.data.e_shnum);
    header.data.e_shstrndx = reverse_2(header.data.e_shstrndx);

    if( header.data.e_ident[EI_MAG0] != ELFMAG0 ||
        header.data.e_ident[EI_MAG1] != ELFMAG1 ||
        header.data.e_ident[EI_MAG2] != ELFMAG2 ||
        header.data.e_ident[EI_MAG3] != ELFMAG3 ){
        fprintf(stderr, "Pas un fichier ELF.\n");
        return 0;
    }

    if (header.data.e_ident[EI_CLASS] != ELFCLASS32) {
        fprintf(stderr, "ELF pas en 32 bits\n");
        return 0;
    }

    if (header.data.e_ident[EI_DATA] != ELFDATA2MSB) {
        fprintf(stderr, "ELF pas en big endian\n");
        return 0;
    }

    if ((header.data.e_machine) != EM_ARM){
        fprintf(stderr, "Pas ARM.\n");
        return 0;
    }

    return 1;
}

