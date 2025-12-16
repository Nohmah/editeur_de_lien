#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>

#include "util.h"
#include "elf_symboles.h"

// inverse une en-tête 
 void inverse_entete(Elf32_Ehdr *ehdr){
    ehdr->e_type = reverse_2(ehdr->e_type);
    ehdr->e_machine = reverse_2(ehdr->e_machine);
    ehdr->e_version = reverse_4(ehdr->e_version);
    ehdr->e_entry = reverse_4(ehdr->e_entry);
    ehdr->e_phoff = reverse_4(ehdr->e_phoff);
    ehdr->e_shoff = reverse_4(ehdr->e_shoff);
    ehdr->e_flags = reverse_4(ehdr->e_flags);
    ehdr->e_ehsize = reverse_2(ehdr->e_ehsize);
    ehdr->e_phentsize = reverse_2(ehdr->e_phentsize);
    ehdr->e_phnum = reverse_2(ehdr->e_phnum);
    ehdr->e_shentsize = reverse_2(ehdr->e_shentsize);
    ehdr->e_shnum = reverse_2(ehdr->e_shnum);
    ehdr->e_shstrndx = reverse_2(ehdr->e_shstrndx);
}

void inverse_ONE_Section(Elf32_Shdr *sh){
    sh->sh_name = reverse_4(sh->sh_name);
    sh->sh_type = reverse_4(sh->sh_type);
    sh->sh_addr = reverse_4(sh->sh_addr);
    sh->sh_offset = reverse_4(sh->sh_offset);
    sh->sh_size = reverse_4(sh->sh_size);
    sh->sh_entsize = reverse_4(sh->sh_entsize);
    sh->sh_flags = reverse_4(sh->sh_flags);
    sh->sh_link = reverse_4(sh->sh_link);
    sh->sh_info = reverse_4(sh->sh_info);
    sh->sh_addralign = reverse_4(sh->sh_addralign);
}

void inverse_Section(Elf32_Shdr *Tabsh, int n){
    for(int i = 0; i < n ; i++){
        inverse_ONE_Section(Tabsh + i);
    }
}

// Lecture de l’en-tête ELF
int lire_entete_elf(elf_contexte_t *ctx)
{
    fseek(ctx->file, 0, SEEK_SET);


    if (fread(&(ctx->ehdr), sizeof(Elf32_Ehdr), 1, ctx->file) != 1) {
        perror("fread entete");
        return -1;
    }

    inverse_entete(&(ctx->ehdr));

    // Vérification ELF
    /*if (ctx->ehdr.e_ident[EI_MAG0] != ELFMAG0 ||
        ctx->ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
        ctx->ehdr.e_ident[EI_MAG2] != ELFMAG2 ||
        ctx->ehdr.e_ident[EI_MAG3] != ELFMAG3) {
        fprintf(stderr, "Pas un fichier ELF\n");
        return -1;
    }*/

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
    //printf("Entrer dans trouver_section\n");
    //printf("section rechercher : %s \n",nom);
    char *nom_courant;
    for (int i = 0; i < ctx->ehdr.e_shnum; i++) {
        nom_courant = ctx->noms_sections + ctx->sections[i].sh_name;
        //printf("section %d = %s \n",i,nom_courant);

        if (strcmp(nom_courant, nom) == 0){
            //printf("TROUVER !!\n");
            return i;
        }
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

    char *nom; 
    for (int i = 0; i < ctx->nb_symboles; i++) {
        nom = ctx->noms_sections + ctx->sections[i].sh_name;
        //nom = ctx->noms_symboles + ctx->symboles[i].st_name;
/*
        printf("\n1%5d \n\n\n",i);
        printf("2%08x \n",ctx->symboles[i].st_value);
        printf("3%-8s \n",type_symbole(ctx->symboles[i].st_info));
        printf("4%-8s \n",portee_symbole(ctx->symboles[i].st_info));
        printf("5%7d \n",ctx->symboles[i].st_shndx);
        printf("6%s \n",nom);
*/
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
