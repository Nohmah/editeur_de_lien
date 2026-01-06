#ifndef ELF_FUSION_SYMBOLES_H
#define ELF_FUSION_SYMBOLES_H

#include <elf.h>
#include "elf_symboles.h"

typedef struct {
  Elf32_Sym *symboles;
  char *noms_symboles;
  int nb_symboles;
} elf_table_sym_t;

typedef struct {
	elf_table_sym_t* locaux;
	elf_table_sym_t* globaux;
  int taille_string;
} elf_table_sep_t;

//Constructeur + Destructeur de elf_table_sym_t : size est le nombre attendu de Elf32_Sym / nb_symboles dedans
elf_table_sym_t* construire_table_sym(int size);
void detruire_table_sym(elf_table_sym_t* table);

elf_table_sym_t* fusion_tables_symboles(elf_contexte_t* ctx1, elf_contexte_t* ctx2);
#endif