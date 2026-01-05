#ifndef __ELF_HEADER__
#define __ELF_HEADER__

#include <elf.h>
#include <stdio.h>
#include "elf_symboles.h"

// Vérifie si le fichier est valide en lisant et stockant l'entete
int verifier_fichier(FILE* f, elf_contexte_t *ctx);

// Affiche l'entête
void afficher_entete(elf_contexte_t *ctx);

#endif