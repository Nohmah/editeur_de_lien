#ifndef ELF_SYMBOLES_H
#define ELF_SYMBOLES_H

#include <elf.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
  FILE *file;           /* fichier ELF ouvert */
  Elf32_Ehdr ehdr;      /* entête ELF */
  Elf32_Shdr *sections; /* table des sections */
  char *noms_sections;  /* table des noms de sections */

  /* table des symboles */
  Elf32_Sym *symboles;
  char *noms_symboles;
  int nb_symboles;
} elf_contexte_t;

/* Lecture ELF de base */
int lire_entete_elf(elf_contexte_t *ctx);
int lire_table_sections(elf_contexte_t *ctx);
int lire_noms_sections(elf_contexte_t *ctx);

/* Recherche d’une section par son nom */
int trouver_section(const elf_contexte_t *ctx, const char *nom);

/* Chargement de la table des symboles dans ctx */
int charger_table_symboles(elf_contexte_t *ctx);

/* Lecture et affichage de la table des symboles */
int lire_table_symboles(elf_contexte_t *ctx);

/* Aide à l’affichage */
const char *type_symbole(uint8_t st_info);
const char *portee_symbole(uint8_t st_info);

/* Accès pour la partie 5 */
uint32_t valeur_symbole(const elf_contexte_t *ctx, int i);
const char *nom_symbole(const elf_contexte_t *ctx, int i);

/* Nettoyage */
void liberer_contexte(elf_contexte_t *ctx);

#endif