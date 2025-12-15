#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <elf.h>

#include "elf_symboles.h"

int lire_entete_elf(elf_contexte_t *ctx)
{
    lseek(ctx->fd, 0, SEEK_SET);
    
    if (read(ctx->fd, &ctx->ehdr, sizeof(Elf32_Ehdr)) != sizeof(Elf32_Ehdr)) {
        perror("read entete");
        return -1;
    }

    // Vérifier que c'est bien un fichier ELF
    if (ctx->ehdr.e_ident[EI_MAG0] != ELFMAG0 ||
        ctx->ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
        ctx->ehdr.e_ident[EI_MAG2] != ELFMAG2 ||
        ctx->ehdr.e_ident[EI_MAG3] != ELFMAG3) {
        fprintf(stderr, "Pas un fichier ELF\n");
        return -1;
    }

    // Vérifier 32 bits
    if (ctx->ehdr.e_ident[EI_CLASS] != ELFCLASS32) {
        fprintf(stderr, "Pas un ELF 32 bits\n");
        return -1;
    }

    // Vérifier ARM
    if (ctx->ehdr.e_machine != EM_ARM) {
        fprintf(stderr, "Pas une architecture ARM\n");
        return -1;
    }

    return 0;
}

int lire_table_sections(elf_contexte_t *ctx)
{
    int taille = ctx->ehdr.e_shnum * sizeof(Elf32_Shdr);
    
    ctx->sections = malloc(taille);
    if (ctx->sections == NULL) {
        perror("malloc sections");
        return -1;
    }

    lseek(ctx->fd, ctx->ehdr.e_shoff, SEEK_SET);
    
    if (read(ctx->fd, ctx->sections, taille) != taille) {
        perror("read sections");
        return -1;
    }

    return 0;
}

int lire_noms_sections(elf_contexte_t *ctx)
{
    Elf32_Shdr *shstrtab;
    
    if (ctx->ehdr.e_shstrndx == SHN_UNDEF) {
        fprintf(stderr, "Pas de table de noms de sections\n");
        return -1;
    }

    shstrtab = &ctx->sections[ctx->ehdr.e_shstrndx];
    
    ctx->noms_sections = malloc(shstrtab->sh_size);
    if (ctx->noms_sections == NULL) {
        perror("malloc noms sections");
        return -1;
    }

    lseek(ctx->fd, shstrtab->sh_offset, SEEK_SET);
    
    if (read(ctx->fd, ctx->noms_sections, shstrtab->sh_size) != shstrtab->sh_size) {
        perror("read noms sections");
        return -1;
    }

    return 0;
}

int trouver_section(const elf_contexte_t *ctx, const char *nom)
{
    int i;
    char *nom_courant;

    for (i = 0; i < ctx->ehdr.e_shnum; i++) {
        nom_courant = ctx->noms_sections + ctx->sections[i].sh_name;
        if (strcmp(nom_courant, nom) == 0) {
            return i;
        }
    }

    return -1;
}

int lire_table_symboles(const elf_contexte_t *ctx)
{
    int idx_symtab;
    int idx_strtab;
    Elf32_Shdr *symtab;
    Elf32_Shdr *strtab;
    Elf32_Sym *symboles;
    char *noms;
    int nb_symboles;
    int i;

    idx_symtab = trouver_section(ctx, ".symtab");
    idx_strtab = trouver_section(ctx, ".strtab");

    if (idx_symtab == -1 || idx_strtab == -1) {
        fprintf(stderr, "Table des symboles non trouvée\n");
        return -1;
    }

    symtab = &ctx->sections[idx_symtab];
    strtab = &ctx->sections[idx_strtab];

    nb_symboles = symtab->sh_size / sizeof(Elf32_Sym);

    symboles = malloc(symtab->sh_size);
    noms = malloc(strtab->sh_size);
    
    if (symboles == NULL || noms == NULL) {
        perror("malloc symboles");
        return -1;
    }

    // Lire la table des symboles
    lseek(ctx->fd, symtab->sh_offset, SEEK_SET);
    if (read(ctx->fd, symboles, symtab->sh_size) != symtab->sh_size) {
        perror("read symboles");
        free(symboles);
        free(noms);
        return -1;
    }

    // Lire la table des noms de symboles
    lseek(ctx->fd, strtab->sh_offset, SEEK_SET);
    if (read(ctx->fd, noms, strtab->sh_size) != strtab->sh_size) {
        perror("read noms symboles");
        free(symboles);
        free(noms);
        return -1;
    }

    // Afficher l'en-tête
    printf("Index  Valeur    Type     Portée   Section  Nom\n");

    // Afficher chaque symbole
    for (i = 0; i < nb_symboles; i++) {
        char *nom_symbole = noms + symboles[i].st_name;
        
        printf("%5d  %08x %-8s %-8s %7d  %s\n",
               i,
               symboles[i].st_value,
               type_symbole(symboles[i].st_info),
               portee_symbole(symboles[i].st_info),
               symboles[i].st_shndx,
               nom_symbole);
    }

    free(symboles);
    free(noms);
    return 0;
}

const char *type_symbole(uint8_t st_info)
{
    int type = ELF32_ST_TYPE(st_info);

    if (type == STT_FUNC)
        return "FUNC";
    if (type == STT_OBJECT)
        return "OBJECT";
    if (type == STT_SECTION)
        return "SECTION";
    if (type == STT_FILE)
        return "FILE";
    
    return "NOTYPE";
}

const char *portee_symbole(uint8_t st_info)
{
    int bind = ELF32_ST_BIND(st_info);

    if (bind == STB_LOCAL)
        return "LOCAL";
    if (bind == STB_GLOBAL)
        return "GLOBAL";
    if (bind == STB_WEAK)
        return "WEAK";
    
    return "UNKNOWN";
}

void liberer_contexte(elf_contexte_t *ctx)
{
    if (ctx->sections != NULL) {
        free(ctx->sections);
    }
    if (ctx->noms_sections != NULL) {
        free(ctx->noms_sections);
    }
}
/* TEST de la partie symboles
int main(int argc, char **argv)
{
    elf_contexte_t ctx;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <fichier.o>\n", argv[0]);
        return EXIT_FAILURE;
    }

    memset(&ctx, 0, sizeof(ctx));

    ctx.fd = open(argv[1], O_RDONLY);
    if (ctx.fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    if (lire_entete_elf(&ctx) < 0) {
        liberer_contexte(&ctx);
        close(ctx.fd);
        return EXIT_FAILURE;
    }

    if (lire_table_sections(&ctx) < 0) {
        liberer_contexte(&ctx);
        close(ctx.fd);
        return EXIT_FAILURE;
    }

    if (lire_noms_sections(&ctx) < 0) {
        liberer_contexte(&ctx);
        close(ctx.fd);
        return EXIT_FAILURE;
    }

    if (lire_table_symboles(&ctx) < 0) {
        liberer_contexte(&ctx);
        close(ctx.fd);
        return EXIT_FAILURE;
    }

    liberer_contexte(&ctx);
    close(ctx.fd);
    return EXIT_SUCCESS;
}*/