#include "elf_section_table.h"
#include <stdio.h>
#include <elf.h>
#include <stdlib.h>
#include <stdint.h>
#include "util.h"
#include "elf_symboles.h"

void afficher_sections_table(const elf_contexte_t *ctx) {
    
    uint32_t shoff = ctx->ehdr.e_shoff;
    uint16_t shnum = ctx->ehdr.e_shnum;
    
    printf("\nThere are %d section headers, starting at offset 0x%x:\n\n", shnum, shoff);
    printf("Section Headers:\n");
    printf("[Nr] Name               Type           Addr     Off    Size   ES Flg Lk Inf Al\n");

    for (int i = 0; i < shnum; i++) {
        Elf32_Shdr *sh = &ctx->sections[i];

        // Récupérer le nom depuis la table des noms
        const char *name = ctx->noms_sections + sh->sh_name;

        uint32_t type  = sh->sh_type;
        uint32_t addr  = sh->sh_addr;
        uint32_t off   = sh->sh_offset;
        uint32_t size  = sh->sh_size;
        uint32_t es    = sh->sh_entsize;
        uint32_t flags = sh->sh_flags;
        uint32_t link  = sh->sh_link;
        uint32_t info  = sh->sh_info;
        uint32_t al    = sh->sh_addralign;

        printf("[%2d] %-18s %-14s %08x %06x %06x %02x ",
               i, name, sh_type_str(type),
               addr, off, size, es);

        print_flags(flags);

        printf(" %2d %3d %2d\n", link, info, al);
    }
}

const char *sh_type_str(uint32_t type) {
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

void print_flags(uint32_t flags) {
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