#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <elf.h>
#include "util.h"
#include "elf_reader.h"

#define FIX16(x) (file_big_endian ? reverse_2(x) : (x))
#define FIX32(x) (file_big_endian ? reverse_4(x) : (x))

 
int main(int argc, char *argv[]){
    if( argc < 2 ){
        fprintf(stderr, "Usage: %s file.o\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if(!f) { perror("fopen"); return 1;}


    Elf32_Ehdr hdr;
    fread(&hdr, 1, sizeof(hdr), f);

    
    /* Check if the readen file is an ELF file. */
    if( hdr.e_ident[EI_MAG0] != ELFMAG0 ||
        hdr.e_ident[EI_MAG1] != ELFMAG1 ||
        hdr.e_ident[EI_MAG2] != ELFMAG2 ||
        hdr.e_ident[EI_MAG3] != ELFMAG3 ){
        fprintf(stderr, "Not an ELF File.\n");
        fclose(f);
        return 1;
    }
    /* Does not allow ELF64, you can disable this condition, the program will works. */
    if (hdr.e_ident[EI_CLASS] != ELFCLASS32) {
        fprintf(stderr, "ELF64 non supporté\n");
        fclose(f);
        return 1;
    }    

    //elf_header(hdr);
    elf_section(f, hdr);
    fclose(f);


    return 0;

}

/* Print ELF header */
void elf_header(Elf32_Ehdr hdr){

    int file_big_endian = (hdr.e_ident[EI_DATA] == ELFDATA2MSB);

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

    printf("Type:   0x%04x %s\n", FIX16(hdr.e_type), (FIX16(hdr.e_type) ==  ET_REL ? "REL (Fichier de réadressage)" : "Autres"));
    printf("Machine:   0x%04x %s\n", FIX16(hdr.e_machine), (FIX16(hdr.e_machine) == EM_ARM ? "ARM" : "Autres"));
    printf("Version:   0x%08x\n", FIX32(hdr.e_version));
    printf("Adresse du point d'entrée:   0x%08x\n", FIX32(hdr.e_entry));

    printf("Début des en-têtes de programme:   %u %s\n", FIX32(hdr.e_phoff), "(octets dans le fichier)");
    printf("Début des en-têtes de section:   %u %s\n", FIX32(hdr.e_shoff), "(octets dans le fichier)");

    printf("Flags:   0x%08x\n", FIX32(hdr.e_flags));

    printf("Taille de cet en-tête: %u %s\n", FIX16(hdr.e_ehsize), "(octets)");
    printf("Taille de l'en-tête du programme:   %u %s\n", FIX16(hdr.e_phentsize), "(octets)");
    printf("Nombre d'en-tête du programme:   %u\n", FIX16(hdr.e_phnum));
    printf("Taille des en-têtes de section:   %u %s\n", FIX16(hdr.e_shentsize), "(octets)");
    printf("Nombre d'en-tête de section:   %u\n", FIX16(hdr.e_shnum));
    printf("Table d'index des chaînes d'en-tête de section:   %u\n", FIX16(hdr.e_shstrndx));

}

void elf_section(FILE *f, Elf32_Ehdr hdr)
{
    int file_big_endian = (hdr.e_ident[EI_DATA] == ELFDATA2MSB);

    uint32_t shoff     = FIX32(hdr.e_shoff);
    uint16_t shnum     = FIX16(hdr.e_shnum);
    uint16_t shentsize = FIX16(hdr.e_shentsize);
    uint16_t shstrndx  = FIX16(hdr.e_shstrndx);

    Elf32_Shdr *sections = malloc(shnum * sizeof(Elf32_Shdr));

    fseek(f, shoff, SEEK_SET);
    for (int i = 0; i < shnum; i++)
        fread(&sections[i], 1, shentsize, f);

    /* lecture de .shstrtab */
    Elf32_Shdr shstr = sections[shstrndx];
    uint32_t off  = FIX32(shstr.sh_offset);
    uint32_t size = FIX32(shstr.sh_size);

    char *shstrtab = malloc(size);
    fseek(f, off, SEEK_SET);
    fread(shstrtab, 1, size, f);

    printf("\nIl y a %d en-têtes de section, débutant à l'adresse 0x%x\n\n",
           shnum, shoff);

    printf("[Nr] Nom               Type       Adr      Off    Taille ES Fan Ln Inf Al\n");

    for (int i = 0; i < shnum; i++) {
        Elf32_Shdr *sh = &sections[i];

        const char *name = shstrtab + FIX32(sh->sh_name);

        uint32_t type  = FIX32(sh->sh_type);
        uint32_t addr  = FIX32(sh->sh_addr);
        uint32_t off   = FIX32(sh->sh_offset);
        uint32_t size  = FIX32(sh->sh_size);
        uint32_t es    = FIX32(sh->sh_entsize);
        uint32_t flags = FIX32(sh->sh_flags);
        uint32_t link  = FIX32(sh->sh_link);
        uint32_t info  = FIX32(sh->sh_info);
        uint32_t al    = FIX32(sh->sh_addralign);

        printf("[%2d] %-17s %-10s %08x %06x %06x %02x ",
               i, name, sh_type_str(type),
               addr, off, size, es);

        print_flags(flags);

        printf(" %2d %3d %2d\n", link, info, al);
    }

    free(shstrtab);
    free(sections);
}

const char *sh_type_str(uint32_t type)
{
    switch (type) {
        case SHT_NULL:          return "NULL";
        case SHT_PROGBITS:      return "PROGBITS";
        case SHT_SYMTAB:        return "SYMTAB";
        case SHT_STRTAB:        return "STRTAB";
        case SHT_RELA:          return "RELA";
        case SHT_HASH:          return "HASH";
        case SHT_DYNAMIC:       return "DYNAMIC";
        case SHT_NOTE:          return "NOTE";
        case SHT_NOBITS:        return "NOBITS";
        case SHT_REL:           return "REL";
        case SHT_SHLIB:         return "SHLIB";
        case SHT_DYNSYM:        return "DYNSYM";
        case SHT_ARM_ATTRIBUTES:return "ARM_ATTR";
        default:                return "UNKNOWN";
    }
}

void print_flags(uint32_t flags)
{
    char out[4] = "   ";
    int i = 0;

    if (flags & SHF_WRITE)     out[i++] = 'W';
    if (flags & SHF_ALLOC)     out[i++] = 'A';
    if (flags & SHF_EXECINSTR) out[i++] = 'X';
    if (flags & SHF_MERGE)     out[i++] = 'M';
    if (flags & SHF_STRINGS)   out[i++] = 'S';
    if (flags & SHF_INFO_LINK) out[i++] = 'I';
    if (flags & SHF_LINK_ORDER)out[i++] = 'L';
    if (flags & SHF_OS_NONCONFORMING) out[i++] = 'O';
    if (flags & SHF_GROUP)     out[i++] = 'G';
    if (flags & SHF_TLS)       out[i++] = 'T';

    printf("%s", out);
}
