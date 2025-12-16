#ifndef ELF_REIMPLANTATION_H
#define ELF_REIMPLANTATION_H

#include <elf.h>
#include <stdio.h>
#include "elf_symboles.h"

/* Cherche les sections de reimplantations, et les affiche */
int lire_sections_reimplantations(elf_contexte_t* ctx);

#endif