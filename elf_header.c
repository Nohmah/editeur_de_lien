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

    printf("ELF Header:\n");
    printf("Magic:   ");
    for( int i = 0; i < EI_NIDENT; i++){
        printf("%02x ", hdr.e_ident[i]);
    }
    printf("\n");

    printf("Class:   %s\n", (hdr.e_ident[EI_CLASS] == ELFCLASS32 ? "ELF32" : (hdr.e_ident[EI_CLASS] == ELFCLASS64 ? "ELF64" : "Inconnu")));
    printf("Data:   %s\n", (hdr.e_ident[EI_DATA] == ELFDATA2LSB ? "Little Endian" : (hdr.e_ident[EI_DATA] == ELFDATA2MSB ? "Big Endian" : "Inconnu")));
    printf("Version:   %d\n", hdr.e_ident[EI_VERSION]);
    printf("OS/ABI:   %d\n", hdr.e_ident[EI_OSABI]);
    printf("ABI Version:   %d\n", hdr.e_ident[EI_ABIVERSION]);

    printf("Type:   %s\n", (reverse_2(hdr.e_type) ==  ET_REL ? "REL (Fichier de réadressage)" : "Autres"));
    printf("Machine:   %s\n",(reverse_2(hdr.e_machine) == EM_ARM ? "ARM" : "Autres"));
    printf("Version:   0x%08x\n", reverse_4(hdr.e_version));
    printf("Entry point address:   0x%08x\n", reverse_4(hdr.e_entry));

    printf("Start of program headers:   %u %s\n", reverse_4(hdr.e_phoff), "(octets dans le fichier)");
    printf("Start of section headers:   %u %s\n", reverse_4(hdr.e_shoff), "(octets dans le fichier)");

    printf("Flags:   0x%08x\n", reverse_4(hdr.e_flags));

    printf("Size of this header: %u %s\n", reverse_2(hdr.e_ehsize), "(octets)");
    printf("Size of program headers:   %u %s\n", reverse_2(hdr.e_phentsize), "(octets)");
    printf("Number of program headers:   %u\n", reverse_2(hdr.e_phnum));
    printf("Size of section headers:   %u %s\n", reverse_2(hdr.e_shentsize), "(octets)");
    printf("Number of section headers:   %u\n", reverse_2(hdr.e_shnum));
    printf("Section header string table index:   %u\n", reverse_2(hdr.e_shstrndx));

}

int check_spec(FILE* f){
    rewind(f);
    Elf32_Ehdr hdr;
    fread(&hdr, 1, sizeof(Elf32_Ehdr), f);
    if( hdr.e_ident[EI_MAG0] != ELFMAG0 ||
        hdr.e_ident[EI_MAG1] != ELFMAG1 ||
        hdr.e_ident[EI_MAG2] != ELFMAG2 ||
        hdr.e_ident[EI_MAG3] != ELFMAG3 ){
        fprintf(stderr, "Not an ELF File.\n");
        return 0;
    }

    if (hdr.e_ident[EI_CLASS] != ELFCLASS32) {
        fprintf(stderr, "Not an ELF32 File\n");
        return 0;
    }

    if (hdr.e_ident[EI_DATA] != ELFDATA2MSB) {
    fprintf(stderr, "ELF non big endian\n");
    return 0;
    }

    if (reverse_2(hdr.e_machine) != EM_ARM){
        fprintf(stderr, "Not a ARM Arch.\n");
        return 0;
    }

    return 1;
}

