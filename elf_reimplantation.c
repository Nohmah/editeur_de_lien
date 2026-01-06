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
    case 0: strcpy(str, "R_ARM_NONE Static Miscellaneous"); break;
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
    case 40: strcpy(str, "R_ARM_V4BX"); break; // Fichiers initiaux le demande
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
    if (!tableau) return NULL;

    int nb_relations = reitab->sh_size / reitab->sh_entsize;
    Elf32_Rel* entree_rel = malloc(reitab->sh_size);
    if (!entree_rel) {
        free(tableau);
        return NULL;
    }

    tableau->nb_entrees = nb_relations;
    tableau->entrees = malloc(sizeof(elf_entree_REL) * nb_relations);
    if (!tableau->entrees) {
        free(entree_rel);
        free(tableau);
        return NULL;
    }

    const char* nom_section = ctx->noms_sections + ctx->sections[index].sh_name;
    tableau->nom = strdup(nom_section);
    if (!tableau->nom) {
        free(tableau->entrees);
        free(entree_rel);
        free(tableau);
        return NULL;
    }

    tableau->offset = reitab->sh_offset;
        
    fseek(ctx->file, reitab->sh_offset, SEEK_SET);
    if (fread(entree_rel, reitab->sh_size, 1, ctx->file) != 1) {
        printf("ERREUR fread REL\n");
        free(tableau->nom);
        free(tableau->entrees);
        free(entree_rel);
        free(tableau);
        exit(1);
    }
    
    for (int i = 0; i < nb_relations; i++)
    {
        inverse_REL(&entree_rel[i]);

        unsigned char type_c = ELF32_R_TYPE(entree_rel[i].r_info);
        int sym_c = ELF32_R_SYM(entree_rel[i].r_info);

        elf_entree_REL* curr_entree = &tableau->entrees[i];

        curr_entree->offset = entree_rel[i].r_offset;
        curr_entree->info   = entree_rel[i].r_info;

        curr_entree->type = malloc(64);
        if (curr_entree->type)
            type_to_string((int)type_c, curr_entree->type);
        else
            curr_entree->type = "ERROR_MALLOC_TYPE";

        const char* nom_sym = nom_symbole(ctx, sym_c);
        if (nom_sym && nom_sym[0] != '\0') {
            curr_entree->nom = strdup(nom_sym);
            if (!curr_entree->nom)
                curr_entree->nom = "ERROR_MALLOC_NOM";
        } else {
            curr_entree->nom = strdup("");
        }

        curr_entree->valeur = valeur_symbole(ctx, sym_c);
    }

    free(entree_rel);
    return tableau;
}

//Affiche les relations d'une table REL
void afficher_table_REL(elf_tableau_REL* tableau)
{
    printf("Section de readressage %s à l'adresse offset 0x%x contient %d entrées : \n", tableau->nom, tableau->offset, tableau->nb_entrees);
    printf("Offset   Info     Type             Sym.Value Sym.Name\n");
    
    for (int i = 0; i < tableau->nb_entrees; i++)
    {
        elf_entree_REL* e = &tableau->entrees[i];

        // Si le nom est vide
        int has_name = (e->nom && e->nom[0] != '\0');

        if (has_name) {
            printf("%08x %08x %-16s %08x %s\n",
                   e->offset, e->info, e->type, e->valeur, e->nom);
        } else {
            printf("%08x %08x %-16s\n",
                   e->offset, e->info, e->type);
        }
    }
}

// Charge les relations d'une table RELA
elf_tableau_RELA* charger_table_RELA(Elf32_Shdr* reitab, elf_contexte_t* ctx, int index) {
    elf_tableau_RELA* tableau = malloc(sizeof(elf_tableau_RELA));
    if (!tableau) return NULL;

    int nb_relations = reitab->sh_size / reitab->sh_entsize;
    Elf32_Rela* entree_rela = malloc(reitab->sh_size);
    if (!entree_rela) {
        free(tableau);
        return NULL;
    }

    tableau->nb_entrees = nb_relations;
    tableau->entrees = malloc(sizeof(elf_entree_RELA) * nb_relations);
    if (!tableau->entrees) {
        free(entree_rela);
        free(tableau);
        return NULL;
    }

    const char* nom_section = ctx->noms_sections + ctx->sections[index].sh_name;
    tableau->nom = strdup(nom_section);
    if (!tableau->nom) {
        free(tableau->entrees);
        free(entree_rela);
        free(tableau);
        return NULL;
    }

    tableau->offset = reitab->sh_offset;
       
    fseek(ctx->file, reitab->sh_offset, SEEK_SET);
    if (fread(entree_rela, reitab->sh_size, 1, ctx->file) != 1) {
        printf("ERREUR fread RELA\n");
        free(tableau->nom);
        free(tableau->entrees);
        free(entree_rela);
        free(tableau);
        exit(1);
    }
   
    for (int i = 0; i < nb_relations; i++)
    {
        inverse_RELA(&entree_rela[i]);

        unsigned char type_c = ELF32_R_TYPE(entree_rela[i].r_info);
        int sym_c = ELF32_R_SYM(entree_rela[i].r_info);

        elf_entree_RELA* curr_entree = &tableau->entrees[i];

        curr_entree->offset = entree_rela[i].r_offset;
        curr_entree->info   = entree_rela[i].r_info;
        curr_entree->addend = (int)entree_rela[i].r_addend;

        curr_entree->type = malloc(64);
        if (curr_entree->type) {
            type_to_string((int)type_c, curr_entree->type);
        } else {
            curr_entree->type = "ERROR_MALLOC_TYPE";
        }
        const char* nom_sym = nom_symbole(ctx, sym_c);
        if (nom_sym && nom_sym[0] != '\0') {
            curr_entree->nom = strdup(nom_sym);
            if (!curr_entree->nom) {
                curr_entree->nom = "ERROR_MALLOC_NOM";
            }
        } else {
            curr_entree->nom = strdup("");
            if (!curr_entree->nom) {
                curr_entree->nom = "";
            }
        }
        curr_entree->valeur = valeur_symbole(ctx, sym_c);
    }

    free(entree_rela);
    return tableau;
}

//Affiche les relations d'une table RELA
void afficher_table_RELA(elf_tableau_RELA* tableau)
{
    printf("Section de readressage %s à l'adresse offset 0x%x contient %d entrées : \n", tableau->nom, tableau->offset, tableau->nb_entrees);
    printf("Offset   Info     Type             Sym.Value Sym.Name + Addend\n");
    
    for (int i = 0; i < tableau->nb_entrees; i++)
    {
        elf_entree_RELA* e = &tableau->entrees[i];

        int has_name = (e->nom && e->nom[0] != '\0');

        if (has_name) {
            printf("%08x %08x %-16s %08x %s %d\n",
                   e->offset, e->info, e->type, e->valeur, e->nom, e->addend);
        } else {
            printf("%08x %08x %-16s %d\n",
                   e->offset, e->info, e->type, e->addend);
        }
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

void liberer_tables_reimplantations(elf_tables_reimplantations* tables)
{
    if (!tables) return;

    // Libération des tables REL
    for (int i = 0; i < tables->nb_REL; i++)
    {
        // Libérer le nom de la section
        free(tables->tableaux_REL[i].nom);

        // Libérer chaque entrée
        for (int j = 0; j < tables->tableaux_REL[i].nb_entrees; j++)
        {
            free(tables->tableaux_REL[i].entrees[j].type);
            free(tables->tableaux_REL[i].entrees[j].nom);
        }
        // Libérer le tableau d'entrées
        free(tables->tableaux_REL[i].entrees);
    }
    free(tables->tableaux_REL);

    for (int i = 0; i < tables->nb_RELA; i++)
    {
        free(tables->tableaux_RELA[i].nom);
        for (int j = 0; j < tables->tableaux_RELA[i].nb_entrees; j++)
        {
            free(tables->tableaux_RELA[i].entrees[j].type);
            free(tables->tableaux_RELA[i].entrees[j].nom);
        }
        free(tables->tableaux_RELA[i].entrees);
    }
    free(tables->tableaux_RELA);
    free(tables);
}