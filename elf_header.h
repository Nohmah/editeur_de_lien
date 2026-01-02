#ifndef __ELF_HEADER__
#define __ELF_HEADER__

#include <elf.h>
#include <stdio.h>

typedef struct {
    Elf32_Ehdr data;
} ElfHeaderData;

extern ElfHeaderData header;

// Vérifie si le fichier est valide en lisant et stockant l'entete
int verifier_fichier(FILE* f);

// Affiche l'entête
void afficher_entete();

#endif