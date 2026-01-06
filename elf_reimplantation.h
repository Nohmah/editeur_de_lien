#ifndef ELF_REIMPLANTATION_H
#define ELF_REIMPLANTATION_H

#include <elf.h>
#include <stdio.h>
#include "elf_symboles.h"

/* Structs pour stocker la table de reimplantations*/
typedef struct {
  int offset;
  int info;
  char* type;
  int valeur;
  char* nom;
} elf_entree_REL;

typedef struct {
  elf_entree_REL *entrees;
  int nb_entrees;
  char* nom;
  int offset;
} elf_tableau_REL;

typedef struct {
  int offset;
  int info;
  char* type;
  int valeur;
  char* nom;
  int addend;
} elf_entree_RELA;

typedef struct {
  elf_entree_RELA *entrees;
  int nb_entrees;
  char* nom;
  int offset;
} elf_tableau_RELA;

typedef struct {
    elf_tableau_REL* tableaux_REL;
    elf_tableau_RELA* tableaux_RELA;
    int nb_REL;
    int nb_RELA;
} elf_tables_reimplantations;

/* Charger et afficher les tables de réimplantations */
elf_tables_reimplantations* charger_tables_reimplantations(elf_contexte_t* ctx);
int afficher_tables_reimplantations(elf_tables_reimplantations* tables);

void liberer_tables_reimplantations(elf_tables_reimplantations* tables);

#endif
