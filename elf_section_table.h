#ifndef __ELF_SECTION_TABLE__
#define __ELF_SECTION_TABLE__

#include <stdio.h>
#include <elf.h>
#include "elf_symboles.h"

void afficher_sections_table(const elf_contexte_t *ctx);

const char *sh_type_str(uint32_t type);
void print_flags(uint32_t flags);

#endif