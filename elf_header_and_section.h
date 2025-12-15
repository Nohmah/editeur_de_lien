#ifndef __ELF_READER__
#define __ELF_READER__

#include <elf.h>
#include <stdio.h>

void elf_header(FILE* f);
void elf_section(FILE* f);
const char *sh_type_str(uint32_t type);
void print_flags(uint32_t flags);
int check_spec(FILE* f);

#endif