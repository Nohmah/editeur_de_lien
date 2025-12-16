#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>

#include "elf_symboles.h"

// Lecture de l’en-tête ELF
int lire_entete_elf(elf_contexte_t *ctx)
{
    fseek(ctx->file, 0, SEEK_SET);

    if (fread(&ctx->ehdr, sizeof(Elf32_Ehdr), 1, ctx->file) != 1) {
        perror("fread entete");
        return -1;
    }

    // Vérification ELF
    if (ctx->ehdr.e_ident[EI_MAG0] != ELFMAG0 ||
        ctx->ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
        ctx->ehdr.e_ident[EI_MAG2] != ELFMAG2 ||
        ctx->ehdr.e_ident[EI_MAG3] != ELFMAG3) {
        fprintf(stderr, "Pas un fichier ELF\n");
        return -1;
    }

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
    for (int i = 0; i < ctx->ehdr.e_shnum; i++) {
        const char *nom_courant =
            ctx->noms_sections + ctx->sections[i].sh_name;

        if (strcmp(nom_courant, nom) == 0)
            return i;
    }

    return -1;
}

// Lecture et affichage de la table des symboles
int lire_table_symboles(elf_contexte_t *ctx)
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

    fseek(ctx->file, strtab->sh_offset, SEEK_SET);
    if (fread(ctx->noms_symboles, strtab->sh_size, 1, ctx->file) != 1) {
        perror("fread noms symboles");
        return -1;
    }

    printf("Index  Valeur    Type     Portée   Section  Nom\n");

    for (int i = 0; i < ctx->nb_symboles; i++) {
        const char *nom =
            ctx->noms_symboles + ctx->symboles[i].st_name;

        printf("%5d  %08x %-8s %-8s %7d  %s\n",
               i,
               ctx->symboles[i].st_value,
               type_symbole(ctx->symboles[i].st_info),
               portee_symbole(ctx->symboles[i].st_info),
               ctx->symboles[i].st_shndx,
               nom);
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
