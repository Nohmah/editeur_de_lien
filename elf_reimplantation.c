#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include "elf_symboles.h"

///////////////////////////////////////////
////////////// A FAIRE !!! ///////////////
/*
- Tester si ça marche (probablement que non mdr)
- Unifier la recherche de table des symboles avec la table des symboles
*/
///////////////////////////////////////////

//Les entrées REL et RELA sont séparées

//Affiche les relations d'une entrée donée de type REL
void afficher_relations_entree_REL(Elf32_Rel* entree_rel)
{
    //Les printfs sont peut-être pas encore corrects, faut cast en string peut-être ?
    printf("%lu ", (unsigned long)entree_rel->r_offset);
    printf("%lu ", (unsigned long)entree_rel->r_info);

    //On extrait le type, et on l'associe à son équivalent texte
    unsigned char type_c = ELF32_R_TYPE(entree_rel->r_info);
    char type_s[16];
    switch (type_c)
    {
        case 0:
            strcpy(type_s, "STT_NOTYPE");
        case 1:
            strcpy(type_s, "STT_OBJECT");
        case 2:
            strcpy(type_s, "STT_FUNC");
        case 3:
            strcpy(type_s, "STT_SECTION");
        case 4:
            strcpy(type_s, "STT_FILE");
        case 13:
            strcpy(type_s, "STT_LOPROC");
        case 15:
            strcpy(type_s, "STT_HIPROC");
    }
    printf("%s ", type_s);

    //Sym. value, sym. name : utiliser table des symboles
    //On extrait l'index de table des symboles, et on l'associe à la table des symboles
    int sym_c = (int)ELF32_R_SYM(entree_rel->r_info);
    elf_contexte_t *ctx = malloc(sizeof(elf_contexte_t));
    printf("%lu ", valeur_symbole(ctx, sym_c));
    printf("%s ", nom_symbole(ctx, sym_c));
    free(ctx);

    //Pas d'addend pour REL
    printf("XX\n"),
}



//Affiche les relations d'une entrée donée de type RELA
void afficher_relations_entree_RELA(Elf32_Rela* entree_rela)
{
    printf("%lu ", (unsigned long)entree_rela->r_offset);
    printf("%lu ", (unsigned long)entree_rela->r_info);

    //On extrait le type, et on l'associe à son équivalent texte
    unsigned char type_c = ELF32_R_TYPE(entree_rela->r_info);
    char type_s[16];
    switch (type_c)
    {
        case 0:
            strcpy(type_s, "STT_NOTYPE");
        case 1:
            strcpy(type_s, "STT_OBJECT");
        case 2:
            strcpy(type_s, "STT_FUNC");
        case 3:
            strcpy(type_s, "STT_SECTION");
        case 4:
            strcpy(type_s, "STT_FILE");
        case 13:
            strcpy(type_s, "STT_LOPROC");
        case 15:
            strcpy(type_s, "STT_HIPROC");
    }
    printf("%s ", type_s);

    //Sym. value, sym. name : utiliser table des symboles
    //On extrait l'index de table des symboles, et on l'associe à la table des symboles
    int sym_c = (int)ELF32_R_SYM(entree_rel->r_info);
    elf_contexte_t *ctx = malloc(sizeof(elf_contexte_t));
    printf("%lu ", valeur_symbole(ctx, sym_c));
    printf("%s ", nom_symbole(ctx, sym_c));
    free(ctx);

    //Addend
    signed long addend = (signed long)entree_rela->r_addend;
    if (addend > 0) printf("+ %ld\n", addend);
    else printf("- %ld\n", -addend);
}

//Itere sur une table de reimplantations, et affiche toutes les entrees
void afficher_table_relations(Elf32_Shdr* reitab, FILE* f)
{
    Elf32_Word type = reitab->sh_type;
    int nb_relations = reitab->sh_size / reitab->sh_entsize;
    fseek(f, reitab->sh_offset, SEEK_SET);
    printf("Offset Info Type Sym.Value Sym.Name + Addend\n");
    if (type == SHT_RELA)
    {
        Elf32_Rela* entree_rela = malloc(sizeof(Elf32_Rela));
        for (int i = 0; i < nb_relations; i++)
        {
            fread(entree_rela, sizeof(Elf32_Rela), 1, f);
            afficher_relations_entree_RELA(entree_rela);
        }
        free (entree_rela);
    }
    else
    {
        Elf32_Rel* entree_rel = malloc(sizeof(Elf32_Rel));
        for (int i = 0; i < nb_relations; i++)
        {
            fread(entree_rel, sizeof(Elf32_Rel), 1, f);
            afficher_relations_entree_REL(entree_rel);
        }
        free (entree_rel);
    }
}