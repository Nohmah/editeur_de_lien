#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>

#include "fonc_inversion.h"
#include "elf_symboles.h"


// Lecture de l’en-tête ELF
int lire_entete_elf(elf_contexte_t *ctx)
{
    fseek(ctx->file, 0, SEEK_SET);


    if (fread(&(ctx->ehdr), sizeof(Elf32_Ehdr), 1, ctx->file) != 1) {
        perror("ERROR fread entete");
        return -1;
    }

    inverse_entete(&(ctx->ehdr));

    // Vérification ELF déjà faite

    return 0;
}

// Lecture de la table des sections
int lire_table_sections(elf_contexte_t *ctx)
{
    int taille = ctx->ehdr.e_shnum * sizeof(Elf32_Shdr);

    ctx->sections = malloc(taille);
    if (!ctx->sections) {
        perror("malloc sections");
        return -1;
    }

    fseek(ctx->file, ctx->ehdr.e_shoff, SEEK_SET);

    if (fread(ctx->sections, taille, 1, ctx->file) != 1) {
        perror("fread sections");
        return -1;
    }

    inverse_Section(ctx->sections, ctx->ehdr.e_shnum);

    return 0;
}

// Lecture de la table des noms de sections
int lire_noms_sections(elf_contexte_t *ctx)
{
    Elf32_Shdr *shstrtab;

    if (ctx->ehdr.e_shstrndx == SHN_UNDEF) {
        fprintf(stderr, "Pas de table des noms de sections\n");
        return -1;
    }

    shstrtab = &ctx->sections[ctx->ehdr.e_shstrndx];

    ctx->noms_sections = malloc(shstrtab->sh_size);
    if (!ctx->noms_sections) {
        perror("malloc noms sections");
        return -1;
    }

    fseek(ctx->file, shstrtab->sh_offset, SEEK_SET);

    if (fread(ctx->noms_sections, shstrtab->sh_size, 1, ctx->file) != 1) {
        perror("fread noms sections");
        return -1;
    }

    return 0;
}

// Recherche d’une section par son nom
int trouver_section(const elf_contexte_t *ctx, const char *nom)
{
    char *nom_courant;
    for (int i = 0; i < ctx->ehdr.e_shnum; i++) {
        nom_courant = ctx->noms_sections + ctx->sections[i].sh_name;

        if (strcmp(nom_courant, nom) == 0){
            return i;
        }
    }

    return -1;
}

// Chargement de la table des symboles
int charger_table_symboles(elf_contexte_t *ctx)
{
    int idx_symtab = trouver_section(ctx, ".symtab");
    int idx_strtab = trouver_section(ctx, ".strtab");

    if (idx_symtab == -1 || idx_strtab == -1) {
        fprintf(stderr, "Table des symboles non trouvée\n");
        return -1;
    }

    Elf32_Shdr *symtab = &ctx->sections[idx_symtab];
    Elf32_Shdr *strtab = &ctx->sections[idx_strtab];

    ctx->nb_symboles = symtab->sh_size / sizeof(Elf32_Sym);

    ctx->symboles = malloc(symtab->sh_size);
    ctx->noms_symboles = malloc(strtab->sh_size);

    if (!ctx->symboles || !ctx->noms_symboles) {
        perror("malloc symboles");
        return -1;
    }

    fseek(ctx->file, symtab->sh_offset, SEEK_SET);
    if (fread(ctx->symboles, symtab->sh_size, 1, ctx->file) != 1) {
        perror("fread symboles");
        return -1;
    }

    inverse_Sym(ctx->symboles, ctx->nb_symboles);

    fseek(ctx->file, strtab->sh_offset, SEEK_SET);
    if (fread(ctx->noms_symboles, strtab->sh_size, 1, ctx->file) != 1) {
        perror("fread noms symboles");
        return -1;
    }
    
    return 0;
}

// Lecture et affichage de la table des symboles
int lire_table_symboles(elf_contexte_t *ctx)
{
    //Si la table n'est pas chargée, la charger
    int chargement_err = 0;
    if (ctx->nb_symboles == 0){
        chargement_err = charger_table_symboles(ctx);
        if (chargement_err == -1) return -1;
    }
    
    printf("Index  Valeur    Type     Portée   Section  Nom\n");

    char *nom; 

    for (int i = 0; i < ctx->nb_symboles; i++) {

        if (ELF32_ST_TYPE(ctx->symboles[i].st_info) == STT_SECTION) {
            nom = ctx->noms_sections + ctx->sections[ctx->symboles[i].st_shndx].sh_name;
        } else if (ctx->symboles[i].st_name != 0) {
            nom = ctx->noms_symboles + ctx->symboles[i].st_name;
            } else {
                nom = "";
            }
            
        const char *section_str = NULL;
        Elf32_Word shndx = ctx->symboles[i].st_shndx;
        
        if (shndx == SHN_UNDEF) {
            section_str = "UND";
        } 
        else if (shndx == SHN_ABS) {
            section_str = "ABS";
        } 
        else if (shndx == SHN_COMMON) {
            section_str = "COM";
        }
        
        if (section_str != NULL) {
            // Section spéciale
            printf("%5d  %08x %-8s %-8s %7s  %s\n",
                   i,
                   ctx->symboles[i].st_value,
                   type_symbole(ctx->symboles[i].st_info),
                   portee_symbole(ctx->symboles[i].st_info),
                   section_str,
                   nom);
        } else {
            // Section normale
            printf("%5d  %08x %-8s %-8s %7d  %s\n",
                   i,
                   ctx->symboles[i].st_value,
                   type_symbole(ctx->symboles[i].st_info),
                   portee_symbole(ctx->symboles[i].st_info),
                   shndx,
                   nom);
        }
    }

    return 0;
}

// Type du symbole sous forme lisible
const char *type_symbole(uint8_t st_info)
{
    switch (ELF32_ST_TYPE(st_info)) {
        case STT_FUNC:    return "FUNC";
        case STT_OBJECT:  return "OBJECT";
        case STT_SECTION: return "SECTION";
        case STT_FILE:    return "FILE";
        default:          return "NOTYPE";
    }
}

// Portée du symbole
const char *portee_symbole(uint8_t st_info)
{
    switch (ELF32_ST_BIND(st_info)) {
        case STB_LOCAL:  return "LOCAL";
        case STB_GLOBAL: return "GLOBAL";
        case STB_WEAK:   return "WEAK";
        default:         return "UNKNOWN";
    }
}

// Valeur d’un symbole par index (partie 5)
uint32_t valeur_symbole(const elf_contexte_t *ctx, int i)
{
    if (i < 0 || i >= ctx->nb_symboles)
        return 0;

    return ctx->symboles[i].st_value;
}

// Nom d’un symbole par index
const char *nom_symbole(const elf_contexte_t *ctx, int i)
{
    if (i < 0 || i >= ctx->nb_symboles)
        return NULL;

    return ctx->noms_symboles + ctx->symboles[i].st_name;
}

// Libération du contexte
void liberer_contexte(elf_contexte_t *ctx)
{
    if (ctx->sections)
        free(ctx->sections);
    if (ctx->noms_sections)
        free(ctx->noms_sections);
    if (ctx->symboles)
        free(ctx->symboles);
    if (ctx->noms_symboles)
        free(ctx->noms_symboles);
}
