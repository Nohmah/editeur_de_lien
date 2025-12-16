#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <string.h>
#include "elf_symboles.h"
#include "elf_reimplantation.h"
#include "util.h"

///////////////////////////////////////////
////////////// A FAIRE !!! ///////////////
/*
- Acceder à la table des symboles
- Crier de joie ?
*/
///////////////////////////////////////////

//PITIE DITES MOI QU'IL EXISTE UNE FONCTION POUR CA
//J'ai l'impression d'être puni et de copier des lignes
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
    case 33: strcpy(str, "R_ARM_ALU_PCREL_15_8"); break;
    case 34: strcpy(str, "R_ARM_ALU_PCREL_23_15"); break;
    case 35: strcpy(str, "R_ARM_LDR_SBREL_11_0_NC"); break;
    case 36: strcpy(str, "R_ARM_ALU_SBREL_19_12_NC"); break;
    case 37: strcpy(str, "R_ARM_ALU_SBREL_27_20_CK"); break;
    case 38: strcpy(str, "R_ARM_TARGET1"); break;
    case 39: strcpy(str, "R_ARM_SBREL31"); break;
    case 40: strcpy(str, "R_ARM_V4BX"); break;
    case 41: strcpy(str, "R_ARM_TARGET2"); break;
    case 42: strcpy(str, "R_ARM_PREL31"); break;
    case 43: strcpy(str, "R_ARM_MOVW_ABS_NC"); break;
    case 44: strcpy(str, "R_ARM_MOVT_ABS"); break;
    case 45: strcpy(str, "R_ARM_MOVW_PREL_NC"); break;
    case 46: strcpy(str, "R_ARM_MOVT_PREL"); break;
    case 47: strcpy(str, "R_ARM_THM_MOVW_ABS_NC"); break;
    case 48: strcpy(str, "R_ARM_THM_MOVT_ABS"); break;
    case 49: strcpy(str, "R_ARM_THM_MOVW_PREL_NC"); break;
    case 50: strcpy(str, "R_ARM_THM_MOVT_PREL"); break;
    case 51: strcpy(str, "R_ARM_THM_JUMP19"); break;
    case 52: strcpy(str, "R_ARM_THM_JUMP6"); break;
    case 53: strcpy(str, "R_ARM_THM_ALU_PREL_11_0"); break;
    case 54: strcpy(str, "R_ARM_THM_PC12"); break; break;
    case 55: strcpy(str, "R_ARM_ABS32_NOI"); break;
    case 56: strcpy(str, "R_ARM_REL32_NOI"); break;
    case 57: strcpy(str, "R_ARM_ALU_PC_G0_NC"); break;
    case 58: strcpy(str, "R_ARM_ALU_PC_G0"); break;
    case 59: strcpy(str, "R_ARM_ALU_PC_G1_NC"); break;
    case 60: strcpy(str, "R_ARM_ALU_PC_G1"); break;
    case 61: strcpy(str, "R_ARM_ALU_PC_G2"); break;
    case 62: strcpy(str, "R_ARM_LDR_PC_G1"); break;
    case 63: strcpy(str, "R_ARM_LDR_PC_G2"); break;
    case 64: strcpy(str, "R_ARM_LDRS_PC_G0"); break;
    case 65: strcpy(str, "R_ARM_LDRS_PC_G1"); break;
    case 66: strcpy(str, "R_ARM_LDRS_PC_G2"); break;
    case 67: strcpy(str, "R_ARM_LDC_PC_G0"); break;
    case 68: strcpy(str, "R_ARM_LDC_PC_G1"); break;
    case 69: strcpy(str, "R_ARM_LDC_PC_G2"); break;
    case 70: strcpy(str, "R_ARM_ALU_SB_G0"); break;
    case 71: strcpy(str, "R_ARM_ALU_SB_G0"); break;
    case 72: strcpy(str, "R_ARM_ALU_SB_G1_NC"); break;
    case 73: strcpy(str, "R_ARM_ALU_SB_G1"); break;
    case 74: strcpy(str, "R_ARM_ALU_SB_G2"); break;
    case 75: strcpy(str, "R_ARM_LDR_SB_G0"); break;
    case 76: strcpy(str, "R_ARM_LDR_SB_G1"); break;
    case 77: strcpy(str, "R_ARM_LDR_SB_G2"); break;
    case 78: strcpy(str, "R_ARM_LDRS_SB_G0"); break;
    case 79: strcpy(str, "R_ARM_LDRS_SB_G1"); break;
    case 80: strcpy(str, "R_ARM_LDRS_SB_G2"); break;
    case 81: strcpy(str, "R_ARM_LDC_SB_G0"); break;
    case 82: strcpy(str, "R_ARM_LDC_SB_G1"); break;
    case 83: strcpy(str, "R_ARM_LDC_SB_G2"); break;
    case 84: strcpy(str, "R_ARM_MOVW_BREL_NC"); break;
    case 85: strcpy(str, "R_ARM_MOVT_BREL"); break;
    case 86: strcpy(str, "R_ARM_MOVW_BREL"); break;
    case 87: strcpy(str, "R_ARM_THM_MOVW_BREL_NC"); break;
    case 88: strcpy(str, "R_ARM_THM_MOVT_BREL"); break;
    case 89: strcpy(str, "R_ARM_THM_MOVW_BREL"); break;
    case 90: strcpy(str, "R_ARM_TLS_GOTDESC"); break;
    case 91: strcpy(str, "R_ARM_TLS_CALL"); break;
    case 92: strcpy(str, "R_ARM_TLS_DESCSEQ"); break;
    case 93: strcpy(str, "R_ARM_THM_TLS_CALL"); break;
    case 94: strcpy(str, "R_ARM_PLT32_ABS"); break;
    case 95: strcpy(str, "R_ARM_GOT_ABS"); break;
    case 96: strcpy(str, "R_ARM_GOT_PREL"); break;
    case 97: strcpy(str, "R_ARM_GOT_BREL12"); break;
    case 98: strcpy(str, "R_ARM_GOTOFF12"); break;
    case 99: strcpy(str, "R_ARM_GOTRELAX"); break;
    case 100: strcpy(str, "R_ARM_GNU_VTENTRY"); break;
    case 101: strcpy(str, "R_ARM_GNU_VTINHERIT"); break;
    case 102: strcpy(str, "R_ARM_THM_JUMP11"); break;
    case 103: strcpy(str, "R_ARM_THM_JUMP8"); break;
    case 104: strcpy(str, "R_ARM_TLS_GD32"); break;
    case 105: strcpy(str, "R_ARM_TLS_LDM32"); break;
    case 106: strcpy(str, "R_ARM_TLS_LDO32"); break;
    case 107: strcpy(str, "R_ARM_TLS_IE32"); break;
    case 108: strcpy(str, "R_ARM_TLS_LE32"); break;
    case 109: strcpy(str, "R_ARM_TLS_LDO12"); break;
    case 110: strcpy(str, "R_ARM_TLS_LE12"); break;
    case 111: strcpy(str, "R_ARM_TLS_IE12GP"); break;
    case 112: strcpy(str, "R_ARM_PRIVATE1"); break;
    case 113: strcpy(str, "R_ARM_PRIVATE2"); break;
    case 114: strcpy(str, "R_ARM_PRIVATE3"); break;
    case 115: strcpy(str, "R_ARM_PRIVATE4"); break;
    case 116: strcpy(str, "R_ARM_PRIVATE5"); break;
    case 117: strcpy(str, "R_ARM_PRIVATE6"); break;
    case 118: strcpy(str, "R_ARM_PRIVATE7"); break;
    case 119: strcpy(str, "R_ARM_PRIVATE8"); break;
    case 120: strcpy(str, "R_ARM_PRIVATE9"); break;
    case 121: strcpy(str, "R_ARM_PRIVATE10"); break;
    case 122: strcpy(str, "R_ARM_PRIVATE11"); break;
    case 123: strcpy(str, "R_ARM_PRIVATE12"); break;
    case 124: strcpy(str, "R_ARM_PRIVATE13"); break;
    case 125: strcpy(str, "R_ARM_PRIVATE14"); break;
    case 126: strcpy(str, "R_ARM_PRIVATE15"); break;
    case 127: strcpy(str, "R_ARM_PRIVATE16"); break;
    case 128: strcpy(str, "R_ARM_ME_TOO"); break;
    case 129: strcpy(str, "R_ARM_THM_TLS_DESCSEQ16"); break;
    case 130: strcpy(str, "R_ARM_THM_TLS_DESCSEQ32"); break;
    case 131: strcpy(str, "R_ARM_THM_GOT_BREL12"); break;
    case 132: strcpy(str, "R_ARM_THM_ALU_ABS_G0_NC"); break;
    case 133: strcpy(str, "R_ARM_THM_ALU_ABS_G1_NC"); break;
    case 134: strcpy(str, "R_ARM_THM_ALU_ABS_G2_NC"); break;
    case 135: strcpy(str, "R_ARM_THM_ALU_ABS_G3"); break;
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

//Pour corriger l'inversement
Elf32_Rel* inverse_REL(Elf32_Rel* rel)
{
    rel->r_offset = reverse_4(rel->r_offset);
    rel->r_info = reverse_4(rel->r_info);
    return rel;
}

Elf32_Rela* inverse_RELA(Elf32_Rela* rela)
{
    rela->r_offset = reverse_4(rela->r_offset);
    rela->r_info = reverse_4(rela->r_info);
    rela->r_addend = reverse_4(rela->r_addend);
    return rela;
}

//Les entrées REL et RELA sont séparées

//Affiche les relations d'une entrée donée de type REL
void afficher_relations_entree_REL(Elf32_Rel* entree_rel, Elf32_Shdr* reitab, elf_contexte_t* ctx)
{
    //On extrait le type, et on l'associe à son équivalent texte
    unsigned char type_c = (unsigned char)entree_rel->r_info;
    char type_s[32];
    type_to_string((int)type_c, type_s);
    
    //Sym. value, sym. name : utiliser table des symboles
    //On extrait l'index de table des symboles, et on l'associe à la table des symboles
    int sym_c = (int)(entree_rel->r_info >> 8);
    //printf("index : %d\n", sym_c);
    int val_s = valeur_symbole(ctx, sym_c);
    const char* nom_s = nom_symbole(ctx, sym_c);
    
    printf("%08x %08x %-16s %08x %s\n",
               entree_rel->r_offset,
               entree_rel->r_info,
               type_s,
               val_s,
               nom_s);
}



//Affiche les relations d'une entrée donée de type RELA
void afficher_relations_entree_RELA(Elf32_Rela* entree_rela, Elf32_Shdr* reitab, elf_contexte_t* ctx)
{
    //On extrait le type, et on l'associe à son équivalent texte
    unsigned char type_c = (unsigned char)entree_rela->r_info;
    char type_s[32];
    type_to_string((int)type_c, type_s);
    
    //Sym. value, sym. name : utiliser table des symboles
    //On extrait l'index de table des symboles, et on l'associe à la table des symboles
    int sym_c = (int)(entree_rela->r_info >> 8);
    //printf("index : %d\n", sym_c);
    int val_s = valeur_symbole(ctx, sym_c);
    const char* nom_s = nom_symbole(ctx, sym_c);
    
    printf("%08x %08x %-16s %08x %-16s",
               entree_rela->r_offset,
               entree_rela->r_info,
               type_s,
               val_s,
               nom_s);
               
    int addend = (int)entree_rela->r_addend;
    if (addend > 0) printf("+ %d\n", addend);
    else printf("- %d\n", -addend);
}

//Itere sur une table de reimplantations, et affiche toutes les entrees
int afficher_table_relations(Elf32_Shdr* reitab, elf_contexte_t* ctx)
{
    Elf32_Word type = reitab->sh_type;
    int nb_relations = reitab->sh_size / reitab->sh_entsize;
    if (type == SHT_RELA)
    {
        Elf32_Rela* entree_rela = malloc(reitab->sh_size);
        
        fseek(ctx->file, reitab->sh_offset, SEEK_SET);
        if (fread(entree_rela, reitab->sh_size, 1, ctx->file) != 1) {
          printf("ERREUR fread RELA\n");
          return -1;
        }
        
        printf("Offset   Info     Type             Sym.Value Sym.Name + Addend\n");
        for (int i = 0; i < nb_relations; i++)
        {
            afficher_relations_entree_RELA(inverse_RELA(entree_rela + i), reitab, ctx);
        }
        free (entree_rela);
    }
    else if (type == SHT_REL)
    {
        Elf32_Rel* entree_rel = malloc(reitab->sh_size);
        
        fseek(ctx->file, reitab->sh_offset, SEEK_SET);
        if (fread(entree_rel, reitab->sh_size, 1, ctx->file) != 1) {
          printf("ERREUR fread REL\n");
          return -1;
        }
        
        printf("Offset   Info     Type             Sym.Value Sym.Name\n");
        for (int i = 0; i < nb_relations; i++)
        {
            afficher_relations_entree_REL(inverse_REL(entree_rel + i), reitab, ctx);
        }
        free (entree_rel);
    }
    else return -1;
    return 0;
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