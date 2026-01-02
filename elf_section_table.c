#include "elf_section_table.h"
#include <stdio.h>
#include <elf.h>
#include <stdlib.h>
#include <stdint.h>
#include "util.h"

void elf_section(FILE *f){

    uint32_t shoff     = header.data.e_shoff;
    uint16_t shnum     = header.data.e_shnum;    
    uint16_t shentsize = header.data.e_shentsize;
    uint16_t shstrndx  = header.data.e_shstrndx;

    Elf32_Shdr *sections = malloc(shnum * sizeof(Elf32_Shdr));

    fseek(f, shoff, SEEK_SET);
    for (int i = 0; i < shnum; i++)
        fread(&sections[i], 1, shentsize, f);

    /* lecture de .shstrtab */
    Elf32_Shdr shstr = sections[shstrndx];
    uint32_t off  = reverse_4(shstr.sh_offset);
    uint32_t size = reverse_4(shstr.sh_size);

    char *shstrtab = malloc(size);
    fseek(f, off, SEEK_SET);
    fread(shstrtab, 1, size, f);

    printf("\nThere are %d section headers, starting at offset 0x%x:\n\n", shnum, shoff);

    printf("Section Headers:\n");
    printf("[Nr] Name               Type           Addr     Off    Size   ES Flg Lk Inf Al\n");

    for (int i = 0; i < shnum; i++) {
        Elf32_Shdr *sh = &sections[i];

        const char *name = shstrtab + reverse_4(sh->sh_name);

        uint32_t type  = reverse_4(sh->sh_type);
        uint32_t addr  = reverse_4(sh->sh_addr);
        uint32_t off   = reverse_4(sh->sh_offset);
        uint32_t size  = reverse_4(sh->sh_size);
        uint32_t es    = reverse_4(sh->sh_entsize);
        uint32_t flags = reverse_4(sh->sh_flags);
        uint32_t link  = reverse_4(sh->sh_link);
        uint32_t info  = reverse_4(sh->sh_info);
        uint32_t al    = reverse_4(sh->sh_addralign);

        printf("[%2d] %-18s %-14s %08x %06x %06x %02x ",
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
        case SHT_ARM_ATTRIBUTES:return "ARM_ATTRIBUTES";
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