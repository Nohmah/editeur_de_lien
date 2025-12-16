#ifndef __ELF_SECTION_TABLE__
#define __ELF_SECTION_TABLE__
#include <stdio.h>
#include <elf.h>

void elf_section(FILE* f);
const char *sh_type_str(uint32_t type);
void print_flags(uint32_t flags);

#endif