#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <string.h>
#include "elf_symboles.h"
//#include "elf_section_table.h"

///////////////////////////////////////////
////////////// A FAIRE !!! ///////////////
/*
- Tester si ça marche (probablement que non mdr)
*/
///////////////////////////////////////////

//Pitié dites moi que je peux mourir une fois que ça sera fini

//Les entrées REL et RELA sont séparées

//Affiche les relations d'une entrée donée de type REL

void afficher_relations_entree_REL(Elf32_Rel* entree_rel, elf_contexte_t* ctx)
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
    printf("%du ", valeur_symbole(ctx, sym_c));
    printf("%s ", nom_symbole(ctx, sym_c));

    //Pas d'addend pour REL
    printf("XX\n");
}



//Affiche les relations d'une entrée donée de type RELA
void afficher_relations_entree_RELA(Elf32_Rela* entree_rela, elf_contexte_t* ctx)
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
    int sym_c = (int)ELF32_R_SYM(entree_rela->r_info);
    printf("%du ", valeur_symbole(ctx, sym_c));
    printf("%s ", nom_symbole(ctx, sym_c));

    //Addend
    signed long addend = (signed long)entree_rela->r_addend;
    if (addend > 0) printf("+ %ld\n", addend);
    else printf("- %ld\n", -addend);
}

//Itere sur une table de reimplantations, et affiche toutes les entrees
void afficher_table_relations(Elf32_Shdr* reitab, elf_contexte_t* ctx)
{
    Elf32_Word type = reitab->sh_type;
    int nb_relations = reitab->sh_size / reitab->sh_entsize;
    fseek(ctx->file, reitab->sh_offset, SEEK_SET);
    printf("Offset Info Type Sym.Value Sym.Name + Addend\n");
    if (type == SHT_RELA)
    {
        Elf32_Rela* entree_rela = malloc(sizeof(Elf32_Rela));
        for (int i = 0; i < nb_relations; i++)
        {
            fread(entree_rela, sizeof(Elf32_Rela), 1, ctx->file);
            afficher_relations_entree_RELA(entree_rela, ctx);
        }
        free (entree_rela);
    }
    else
    {
        Elf32_Rel* entree_rel = malloc(sizeof(Elf32_Rel));
        for (int i = 0; i < nb_relations; i++)
        {
            fread(entree_rel, sizeof(Elf32_Rel), 1, ctx->file);
            afficher_relations_entree_REL(entree_rel, ctx);
        }
        free (entree_rel);
    }
}

int lire_sections_reimplantations(elf_contexte_t* ctx)
{
  int idx_reitab = trouver_section(ctx, ".rel.text");

  if (idx_reitab == -1) {
    fprintf(stderr, "Table des reimplantations non trouvée \n");
    return -1;
  }

  Elf32_Shdr *reitab = &ctx->sections[idx_reitab];
  afficher_table_relations(reitab, ctx);
  return 0;
}

/*int main()
{
  FILE*testfile = fopen("file1.o", "r");
  //elf_section(testfile);
  //fclose(testfile);
  //testfile = fopen("file1.o", "r");
  
  elf_contexte_t *ctx = malloc(sizeof(elf_contexte_t));
  ctx->file = testfile;
  
  lire_entete_elf(ctx);
  printf("Lire table sections : %d\n", lire_table_sections(ctx));
  lire_noms_sections(ctx);
  lire_sections_reimplantations(ctx);

  fclose(testfile);  
  liberer_contexte(ctx);
}*/