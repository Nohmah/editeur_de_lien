#ifndef ELF_REIMPLANTATION_H
#define ELF_REIMPLANTATION_H

#include <elf.h>
#include <stdio.h>
#include "elf_symboles.h"

/* Cherche les sections de reimplantations, et les affiche */
int lire_sections_reimplantations(elf_contexte_t* ctx);

char* type_to_string(int type, char* str);

/*fonction pour afficher*/
void afficher_relations_entree_REL(Elf32_Rel* entree_rel, Elf32_Shdr* reitab, elf_contexte_t* ctx);
void afficher_relations_entree_RELA(Elf32_Rela* entree_rela, Elf32_Shdr* reitab, elf_contexte_t* ctx);
int afficher_table_relations(Elf32_Shdr* reitab, elf_contexte_t* ctx, char* reitab_name);
#endif
