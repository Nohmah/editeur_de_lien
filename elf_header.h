#ifndef __ELF_HEADER__
#define __ELF_HEADER__

#include <elf.h>
#include <stdio.h>

void elf_header(FILE* f);
int check_spec(FILE* f);

#endif