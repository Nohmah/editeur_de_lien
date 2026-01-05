#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <string.h>
#include "elf_symboles.h"
#include "elf_reimplantation.h"
#include "util.h"
#include "fonc_inversion.h"

// A MODIFIER : Le chargement une fois que j'aurai la struct des sections

char* type_to_string(int type, char* str)
{
    switch(type)
    {
    case 0: strcpy(str, "R_ARM_NONE Static Miscellaneous");
    case 1: strcpy(str, "R_ARM_PC24"); break;
    case 2: strcpy(str, "R_ARM_ABS32"); break;
    case 3: strcpy(str, "R_ARM_REL32"); break;
    case 4: strcpy(str, "R_ARM_LDR_PC_G0"); break;
    case 5: strcpy(str, "R_ARM_ABS16"); break;
    case 6: strcpy(str, "R_ARM_ABS12"); break;
    case 7: strcpy(str, "R_ARM_THM_ABS5"); break;
    case 8: strcpy(str, "R_ARM_ABS8"); break;
    case 9: strcpy(str, "R_ARM_SBREL32"); break;
    case 10: strcpy(str, "R_ARM_THM_CALL"); break;
    case 11: strcpy(str, "R_ARM_THM_PC8"); break;
    case 12: strcpy(str, "R_ARM_BREL_ADJ"); break;
    case 13: strcpy(str, "R_ARM_TLS_DES"); break;
    case 14: strcpy(str, "R_ARM_THM_SWI8"); break;
    case 16: strcpy(str, "R_ARM_THM_XPC22"); break;
    case 17: strcpy(str, "R_ARM_TLS_DTPMOD32"); break;
    case 18: strcpy(str, "R_ARM_TLS_DTPOFF32"); break;
    case 19: strcpy(str, "R_ARM_TLS_TPOFF32"); break;
    case 20: strcpy(str, "R_ARM_COPY"); break;
    case 21: strcpy(str, "R_ARM_GLOB_DAT"); break;
    case 22: strcpy(str, "R_ARM_JUMP_SLOT"); break;
    case 23: strcpy(str, "R_ARM_RELATIVE"); break;
    case 24: strcpy(str, "R_ARM_GOTOFF32"); break;
    case 25: strcpy(str, "R_ARM_BASE_PREL"); break;
    case 26: strcpy(str, "R_ARM_GOT_BREL"); break;
    case 27: strcpy(str, "R_ARM_PLT32"); break;
    case 28: strcpy(str, "R_ARM_CALL"); break;
    case 29: strcpy(str, "R_ARM_JUMP24"); break;
    case 30: strcpy(str, "R_ARM_THM_JUMP24"); break;
    case 31: strcpy(str, "R_ARM_BASE_ABS"); break;
    case 32: strcpy(str, "R_ARM_ALU_PCREL_7_0"); break;
    default : strcpy(str, "ERREUR_TYPE"); break;
  }
  return str;
}

//Je garde ce switch au cas où
/*switch (type_c)
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
        default:
            strcpy(type_s, "ERREUR_TYPE");
    }*/


//Les entrées REL et RELA sont séparées

//Charge les relations d'une table REL
elf_tableau_REL* charger_table_REL(Elf32_Shdr* reitab, elf_contexte_t* ctx, int index) {
    elf_tableau_REL* tableau = malloc(sizeof(elf_tableau_REL));

    int nb_relations = reitab->sh_size / reitab->sh_entsize;
    Elf32_Rel* entree_rel = malloc(reitab->sh_size);

    tableau->nb_entrees = nb_relations;
    tableau->entrees = malloc(sizeof(elf_entree_REL) * nb_relations);
    
    char nom[32];
    strcpy(nom, ctx->noms_sections + index);
    tableau->nom = nom;
    tableau->offset = reitab->sh_offset;
        
    fseek(ctx->file, reitab->sh_offset, SEEK_SET);
    if (fread(entree_rel, reitab->sh_size, 1, ctx->file) != 1) {
        printf("ERREUR fread REL\n");
        exit(1);
    }
    
    Elf32_Rel* curr_rel;
    elf_entree_REL* curr_entree;
    unsigned char type_c;
    char* type_s = malloc(sizeof(char) * 32);
    int sym_c;
    int val_s;
    char* nom_s = malloc(sizeof(char) * 32);
    for (int i = 0; i < nb_relations; i++)
    {
        curr_rel = inverse_REL(&entree_rel[i]);
        curr_entree = malloc(sizeof(elf_entree_REL));

        //On extrait le type, et on l'associe à son équivalent texte
        type_c = (unsigned char)curr_rel->r_info;
        type_to_string((int)type_c, type_s);
    
        //On extrait l'index de table des symboles, et on l'associe à la table des symboles
        sym_c = (int)(curr_rel->r_info >> 8);
        val_s = valeur_symbole(ctx, sym_c);
        strcpy(nom_s, nom_symbole(ctx, sym_c));

        curr_entree->offset = curr_rel->r_offset;
        curr_entree->info = curr_rel->r_info;
        curr_entree->type = type_s;
        curr_entree->valeur = val_s;
        curr_entree->nom = nom_s;

        tableau->entrees[i] = *curr_entree;
    }
    free (curr_rel);
    return tableau;
}

//Affiche les relations d'une table REL
void afficher_table_REL(elf_tableau_REL* tableau)
{
    printf("Section de readressage %s à l'adresse offset 0x%x contient %d entrées : \n", tableau->nom, tableau->offset, tableau->nb_entrees);
    printf("Offset   Info     Type             Sym.Value Sym.Name\n");
    
    elf_entree_REL* entree;
    for (int i = 0; i < tableau->nb_entrees; i++)
    {
        entree = &tableau->entrees[i];
        printf("%08x %08x %-16s %08x %s\n",
            entree->offset,
            entree->info,
            entree->type,
            entree->valeur,
            entree->nom);
    }
}

//Charge les relations d'une table RELA
elf_tableau_RELA* charger_table_RELA(Elf32_Shdr* reitab, elf_contexte_t* ctx, int index) {
    elf_tableau_RELA* tableau = malloc(sizeof(elf_tableau_RELA));

    int nb_relations = reitab->sh_size / reitab->sh_entsize;
    Elf32_Rela* entree_rela = malloc(reitab->sh_size);

    tableau->nb_entrees = nb_relations;
    tableau->entrees = malloc(sizeof(elf_entree_RELA) * nb_relations);
    
    char nom[32];
    strcpy(nom, ctx->noms_sections + index);
    tableau->nom = nom;
    tableau->offset = reitab->sh_offset;
        
    fseek(ctx->file, reitab->sh_offset, SEEK_SET);
    if (fread(entree_rela, reitab->sh_size, 1, ctx->file) != 1) {
        printf("ERREUR fread RELA\n");
        exit(1);
    }
    
    Elf32_Rela* curr_rela;
    elf_entree_RELA* curr_entree;
    unsigned char type_c;
    char* type_s = malloc(sizeof(char) * 32);
    int sym_c;
    int val_s;
    char* nom_s = malloc(sizeof(char) * 32);
    for (int i = 0; i < nb_relations; i++)
    {
        curr_rela = inverse_RELA(&entree_rela[i]);
        curr_entree = malloc(sizeof(elf_entree_RELA));

        //On extrait le type, et on l'associe à son équivalent texte
        type_c = (unsigned char)curr_rela->r_info;
        type_to_string((int)type_c, type_s);
    
        //On extrait l'index de table des symboles, et on l'associe à la table des symboles
        sym_c = (int)(curr_rela->r_info >> 8);
        val_s = valeur_symbole(ctx, sym_c);
        strcpy(nom_s, nom_symbole(ctx, sym_c));

        curr_entree->offset = curr_rela->r_offset;
        curr_entree->info = curr_rela->r_info;
        curr_entree->type = type_s;
        curr_entree->valeur = val_s;
        curr_entree->nom = nom_s;
        curr_entree->addend = (int)curr_rela->r_addend;

        tableau->entrees[i] = *curr_entree;
    }
    free (curr_rela);
    return tableau;
}

//Affiche les relations d'une table RELA
void afficher_table_RELA(elf_tableau_RELA* tableau)
{
    printf("Section de readressage %s à l'adresse offset 0x%x contient %d entrées : \n", tableau->nom, tableau->offset, tableau->nb_entrees);
    printf("Offset   Info     Type             Sym.Value Sym.Name + Addend\n");
    
    elf_entree_RELA* entree;
    for (int i = 0; i < tableau->nb_entrees; i++)
    {
        entree = &tableau->entrees[i];
        printf("%08x %08x %-16s %08x %s %d \n",
            entree->offset,
            entree->info,
            entree->type,
            entree->valeur,
            entree->nom,
            entree->addend);
    }
}

// A réécrire une fois que la table des sections sera séparées
// Trouver toutes les sections REL et RELA, et les mettre dans une struct
elf_tables_reimplantations* charger_tables_reimplantations(elf_contexte_t* ctx)
{
    elf_tables_reimplantations* tables = malloc(sizeof(elf_tables_reimplantations));
    int nb_REL = 0;
    int nb_RELA = 0;
    
    int nb_sections = ctx->ehdr.e_shnum;
    Elf32_Shdr* section;
    for (int i = 0; i < nb_sections; i++)
    {
        section = &ctx->sections[i];
        if (section->sh_type == 9) nb_REL++;
        if (section->sh_type == 4) nb_RELA++;
    }
    
    tables->tableaux_REL = malloc(sizeof(elf_tableau_REL) * nb_REL);
    tables->tableaux_RELA = malloc(sizeof(elf_tableau_RELA) * nb_RELA);
    
    int REL_i = 0;
    int RELA_i = 0;
    for (int i = 0; i < nb_sections; i++)
    {
        section = &ctx->sections[i];
        if (section->sh_type == 9)
        {
          tables->tableaux_REL[REL_i] = *charger_table_REL(section, ctx, i);
          REL_i++;
        }
        if (section->sh_type == 4)
        {
          tables->tableaux_RELA[RELA_i] = *charger_table_RELA(section, ctx, i);
          RELA_i++;
        }
    }

    tables->nb_REL = nb_REL;
    tables->nb_RELA = nb_RELA;
    return tables;
}

int afficher_tables_reimplantations(elf_tables_reimplantations* tables)
{
    printf("Tables de reimplantations : %d REL, %d RELA\n", tables->nb_REL, tables->nb_RELA);
    for (int i = 0; i < tables->nb_REL; i++)
    {
        afficher_table_REL(&tables->tableaux_REL[i]);
    }
    for (int i = 0; i < tables->nb_RELA; i++)
    {
        afficher_table_RELA(&tables->tableaux_RELA[i]);
    }
    return 0;
}
