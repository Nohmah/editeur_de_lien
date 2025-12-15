#ifndef __ELF_READER__
#define __ELF_READER__

#include <elf.h>
#include <stdio.h>

void elf_header(Elf32_Ehdr hdr);
void elf_section(FILE *f, Elf32_Ehdr hdr);
const char *sh_type_str(uint32_t type);
void print_flags(uint32_t flags);

#endif