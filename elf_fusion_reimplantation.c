#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include "elf_fusion_reimplantation.h"
#include "elf_reimplantation.h"
#include "elf_symboles.h"
#include "fusion_table.h"
#include "fonc_inversion.h"

// Structure pour stocker la correspondance des symboles
typedef struct {
    int *map_ctx1;  // map_ctx1[old_idx] = new_idx
    int *map_ctx2;  // map_ctx2[old_idx] = new_idx
    int nb_sym_ctx1;
    int nb_sym_ctx2;
} symbol_mapping_t;

// Crée le mapping des symboles anciens vers nouveaux indices
symbol_mapping_t* creer_mapping_symboles(elf_contexte_t *ctx1, elf_contexte_t *ctx2, elf_table_sym_t *table_fusion) {
    symbol_mapping_t *mapping = malloc(sizeof(symbol_mapping_t));
    if (!mapping) return NULL;
    
    mapping->nb_sym_ctx1 = ctx1->nb_symboles;
    mapping->nb_sym_ctx2 = ctx2->nb_symboles;
    
    mapping->map_ctx1 = malloc(ctx1->nb_symboles * sizeof(int));
    mapping->map_ctx2 = malloc(ctx2->nb_symboles * sizeof(int));
    
    if (!mapping->map_ctx1 || !mapping->map_ctx2) {
        free(mapping->map_ctx1);
        free(mapping->map_ctx2);
        free(mapping);
        return NULL;
    }
    
    // Initialiser à -1 (non trouvé)
    for (int i = 0; i < ctx1->nb_symboles; i++) {
        mapping->map_ctx1[i] = -1;
    }
    for (int i = 0; i < ctx2->nb_symboles; i++) {
        mapping->map_ctx2[i] = -1;
    }
    
    // Mapper les symboles de ctx1
    int offset_str_ctx1 = 0;
    for (int i = 0; i < ctx1->nb_symboles; i++) {
        const char *nom_ctx1 = ctx1->noms_symboles + ctx1->symboles[i].st_name;
        
        for (int j = 0; j < table_fusion->nb_symboles; j++) {
            const char *nom_fusion = table_fusion->noms_symboles + table_fusion->symboles[j].st_name;
            
            if (strcmp(nom_ctx1, nom_fusion) == 0 &&
                ctx1->symboles[i].st_value == table_fusion->symboles[j].st_value &&
                ctx1->symboles[i].st_shndx == table_fusion->symboles[j].st_shndx) {
                mapping->map_ctx1[i] = j;
                break;
            }
        }
    }
    
    // Calculer l'offset de la string table de ctx1
    for (int i = 0; i < ctx1->nb_symboles; i++) {
        offset_str_ctx1 += strlen(ctx1->noms_symboles + ctx1->symboles[i].st_name) + 1;
    }
    
    // Mapper les symboles de ctx2
    for (int i = 0; i < ctx2->nb_symboles; i++) {
        const char *nom_ctx2 = ctx2->noms_symboles + ctx2->symboles[i].st_name;
        
        for (int j = 0; j < table_fusion->nb_symboles; j++) {
            const char *nom_fusion = table_fusion->noms_symboles + table_fusion->symboles[j].st_name;
            
            // Comparer le nom avec et sans offset
            int match = 0;
            if (strcmp(nom_ctx2, nom_fusion) == 0) {
                match = 1;
            } else if (table_fusion->symboles[j].st_name >= offset_str_ctx1) {
                // Ce symbole vient potentiellement de ctx2
                const char *nom_fusion_offset = table_fusion->noms_symboles + 
                    (table_fusion->symboles[j].st_name);
                if (strcmp(nom_ctx2, nom_fusion_offset) == 0) {
                    match = 1;
                }
            }
            
            if (match) {
                mapping->map_ctx2[i] = j;
                break;
            }
        }
    }
    
    return mapping;
}

void liberer_mapping_symboles(symbol_mapping_t *mapping) {
    if (!mapping) return;
    free(mapping->map_ctx1);
    free(mapping->map_ctx2);
    free(mapping);
}

// Trouve l'index de sortie d'une section donnée
int trouver_idx_out(section_fusion_t *fusion, int fusion_count, int idx_ctx, int from_ctx1) {
    for (int i = 0; i < fusion_count; i++) {
        if (from_ctx1) {
            if (fusion[i].idx_ctx1 == idx_ctx) {
                return fusion[i].idx_out;
            }
        } else {
            if (fusion[i].idx_ctx2 == idx_ctx) {
                return fusion[i].idx_out;
            }
        }
    }
    return -1;
}


// Trouve l'offset de concaténation pour une section
uint32_t trouver_offset_concat(section_fusion_t *fusion, int fusion_count, int idx_ctx, int from_ctx1) {
    for (int i = 0; i < fusion_count; i++) {
        if (from_ctx1) {
            if (fusion[i].idx_ctx1 == idx_ctx) {
                return 0; // Les sections de ctx1 ne sont pas décalées
            }
        } else {
            if (fusion[i].idx_ctx2 == idx_ctx) {
                return fusion[i].offset;
            }
        }
    }
    return 0;
}

// Lit un mot de 32 bits depuis une section
int32_t lire_addend_section(elf_contexte_t *ctx, int section_idx, uint32_t offset) {
    if (section_idx < 0 || section_idx >= ctx->ehdr.e_shnum) {
        return 0;
    }
    
    Elf32_Shdr *section = &ctx->sections[section_idx];
    if (offset + 4 > section->sh_size) {
        return 0;
    }
    
    uint32_t file_offset = section->sh_offset + offset;
    fseek(ctx->file, file_offset, SEEK_SET);
    
    int32_t value;
    if (fread(&value, 4, 1, ctx->file) != 1) {
        return 0;
    }
    
    // Inverser pour big endian
    value = __builtin_bswap32(value);
    
    return value;
}

// Corrige une entrée de relocation REL
void corriger_entree_REL(elf_entree_REL *entree, elf_contexte_t *ctx, int from_ctx1,
                         section_fusion_t *fusion, int fusion_count,
                         symbol_mapping_t *sym_map, elf_table_sym_t *table_fusion) {
    
    // 1. Corriger l'offset de la réimplantation
    int old_sym_idx = ELF32_R_SYM(entree->info);
    
    // Trouver la section concernée par cette relocation
    // L'offset est relatif à la section qui contient cette relocation
    uint32_t offset_concat = 0;
    if (!from_ctx1) {
        // Pour ctx2, on doit ajouter l'offset de concaténation
        // La section est déterminée par le contexte de la table de relocation
        offset_concat = trouver_offset_concat(fusion, fusion_count, 
                                              from_ctx1 ? ctx->symboles[old_sym_idx].st_shndx : ctx->symboles[old_sym_idx].st_shndx, 
                                              from_ctx1);
    }
    
    entree->offset += offset_concat;
    
    // 2. Corriger le numéro de symbole
    int new_sym_idx = -1;
    if (from_ctx1) {
        if (old_sym_idx >= 0 && old_sym_idx < sym_map->nb_sym_ctx1) {
            new_sym_idx = sym_map->map_ctx1[old_sym_idx];
        }
    } else {
        if (old_sym_idx >= 0 && old_sym_idx < sym_map->nb_sym_ctx2) {
            new_sym_idx = sym_map->map_ctx2[old_sym_idx];
        }
    }
    
    if (new_sym_idx != -1) {
        unsigned char type = ELF32_R_TYPE(entree->info);
        entree->info = ELF32_R_INFO(new_sym_idx, type);
    }
}

// Corrige une entrée de relocation RELA
void corriger_entree_RELA(elf_entree_RELA *entree, elf_contexte_t *ctx, int from_ctx1,
                          section_fusion_t *fusion, int fusion_count,
                          symbol_mapping_t *sym_map, elf_table_sym_t *table_fusion) {
    
    // 1. Corriger l'offset
    int old_sym_idx = ELF32_R_SYM(entree->info);
    Elf32_Sym *old_sym = from_ctx1 ? &ctx->symboles[old_sym_idx] : &ctx->symboles[old_sym_idx];
    
    uint32_t offset_concat = 0;
    if (!from_ctx1) {
        offset_concat = trouver_offset_concat(fusion, fusion_count, 
                                              old_sym->st_shndx, from_ctx1);
    }
    
    entree->offset += offset_concat;
    
    // 2. Corriger le numéro de symbole
    int new_sym_idx = -1;
    if (from_ctx1) {
        if (old_sym_idx >= 0 && old_sym_idx < sym_map->nb_sym_ctx1) {
            new_sym_idx = sym_map->map_ctx1[old_sym_idx];
        }
    } else {
        if (old_sym_idx >= 0 && old_sym_idx < sym_map->nb_sym_ctx2) {
            new_sym_idx = sym_map->map_ctx2[old_sym_idx];
        }
    }
    
    if (new_sym_idx != -1) {
        unsigned char type = ELF32_R_TYPE(entree->info);
        entree->info = ELF32_R_INFO(new_sym_idx, type);
        
        // 3. Corriger l'addend si c'est un symbole SECTION
        Elf32_Sym *new_sym = &table_fusion->symboles[new_sym_idx];
        if (ELF32_ST_TYPE(new_sym->st_info) == STT_SECTION) {
            uint32_t section_offset_concat = trouver_offset_concat(fusion, fusion_count,
                                                                    new_sym->st_shndx, from_ctx1);
            
            // Pour R_ARM_ABS*, addend est directement ajouté
            // Pour R_ARM_JUMP24 et R_ARM_CALL, addend est en instructions (divisé par 4)
            unsigned char rel_type = ELF32_R_TYPE(entree->info);
            if (rel_type == R_ARM_JUMP24 || rel_type == R_ARM_CALL || rel_type == 1) { // R_ARM_PC24
                entree->addend += section_offset_concat / 4;
            } else {
                entree->addend += section_offset_concat;
            }
        }
    }
}

// Fusionne les tables de réimplantation
elf_tables_reimplantations* fusionner_tables_reimplantations(
    elf_contexte_t *ctx1, 
    elf_contexte_t *ctx2,
    elf_tables_reimplantations *tables1,
    elf_tables_reimplantations *tables2,
    section_fusion_t *fusion,
    int fusion_count,
    elf_table_sym_t *table_sym_fusion) {
    
    // Créer le mapping des symboles
    symbol_mapping_t *sym_map = creer_mapping_symboles(ctx1, ctx2, table_sym_fusion);
    if (!sym_map) {
        fprintf(stderr, "Erreur: impossible de créer le mapping des symboles\n");
        return NULL;
    }
    
    elf_tables_reimplantations *result = malloc(sizeof(elf_tables_reimplantations));
    if (!result) {
        liberer_mapping_symboles(sym_map);
        return NULL;
    }
    
    // Compter les tables du même nom
    int total_REL = 0;
    int total_RELA = 0;
    
    for (int i = 0; i < tables1->nb_REL; i++) {
        total_REL++;
        // Chercher si tables2 a la même table
        for (int j = 0; j < tables2->nb_REL; j++) {
            if (strcmp(tables1->tableaux_REL[i].nom, tables2->tableaux_REL[j].nom) == 0) {
                // On va fusionner, donc pas de table supplémentaire
                total_REL--;
                break;
            }
        }
    }
    total_REL += tables2->nb_REL;
    
    for (int i = 0; i < tables1->nb_RELA; i++) {
        total_RELA++;
        for (int j = 0; j < tables2->nb_RELA; j++) {
            if (strcmp(tables1->tableaux_RELA[i].nom, tables2->tableaux_RELA[j].nom) == 0) {
                total_RELA--;
                break;
            }
        }
    }
    total_RELA += tables2->nb_RELA;
    
    result->nb_REL = total_REL;
    result->nb_RELA = total_RELA;
    result->tableaux_REL = malloc(sizeof(elf_tableau_REL) * total_REL);
    result->tableaux_RELA = malloc(sizeof(elf_tableau_RELA) * total_RELA);
    
    if (!result->tableaux_REL || !result->tableaux_RELA) {
        free(result->tableaux_REL);
        free(result->tableaux_RELA);
        free(result);
        liberer_mapping_symboles(sym_map);
        return NULL;
    }
    
    int idx_REL = 0;
    int idx_RELA = 0;
    
    // Fusionner les tables REL
    for (int i = 0; i < tables1->nb_REL; i++) {
        elf_tableau_REL *t1 = &tables1->tableaux_REL[i];
        elf_tableau_REL *match = NULL;
        
        // Chercher une table correspondante dans tables2
        for (int j = 0; j < tables2->nb_REL; j++) {
            if (strcmp(t1->nom, tables2->tableaux_REL[j].nom) == 0) {
                match = &tables2->tableaux_REL[j];
                break;
            }
        }
        
        if (match) {
            // Fusionner t1 et match
            result->tableaux_REL[idx_REL].nom = strdup(t1->nom);
            result->tableaux_REL[idx_REL].nb_entrees = t1->nb_entrees + match->nb_entrees;
            result->tableaux_REL[idx_REL].entrees = malloc(
                sizeof(elf_entree_REL) * result->tableaux_REL[idx_REL].nb_entrees
            );
            
            // Copier et corriger les entrées de t1
            for (int k = 0; k < t1->nb_entrees; k++) {
                result->tableaux_REL[idx_REL].entrees[k] = t1->entrees[k];
                corriger_entree_REL(&result->tableaux_REL[idx_REL].entrees[k],
                                   ctx1, 1, fusion, fusion_count, sym_map, table_sym_fusion);
            }
            
            // Copier et corriger les entrées de match
            for (int k = 0; k < match->nb_entrees; k++) {
                result->tableaux_REL[idx_REL].entrees[t1->nb_entrees + k] = match->entrees[k];
                corriger_entree_REL(&result->tableaux_REL[idx_REL].entrees[t1->nb_entrees + k],
                                   ctx2, 0, fusion, fusion_count, sym_map, table_sym_fusion);
            }
            
            idx_REL++;
        } else {
            // Juste copier t1
            result->tableaux_REL[idx_REL].nom = strdup(t1->nom);
            result->tableaux_REL[idx_REL].nb_entrees = t1->nb_entrees;
            result->tableaux_REL[idx_REL].entrees = malloc(sizeof(elf_entree_REL) * t1->nb_entrees);
            
            for (int k = 0; k < t1->nb_entrees; k++) {
                result->tableaux_REL[idx_REL].entrees[k] = t1->entrees[k];
                corriger_entree_REL(&result->tableaux_REL[idx_REL].entrees[k],
                                   ctx1, 1, fusion, fusion_count, sym_map, table_sym_fusion);
            }
            
            idx_REL++;
        }
    }
    
    // Ajouter les tables REL de tables2 qui n'ont pas été fusionnées
    for (int i = 0; i < tables2->nb_REL; i++) {
        elf_tableau_REL *t2 = &tables2->tableaux_REL[i];
        int deja_traite = 0;
        
        for (int j = 0; j < tables1->nb_REL; j++) {
            if (strcmp(t2->nom, tables1->tableaux_REL[j].nom) == 0) {
                deja_traite = 1;
                break;
            }
        }
        
        if (!deja_traite) {
            result->tableaux_REL[idx_REL].nom = strdup(t2->nom);
            result->tableaux_REL[idx_REL].nb_entrees = t2->nb_entrees;
            result->tableaux_REL[idx_REL].entrees = malloc(sizeof(elf_entree_REL) * t2->nb_entrees);
            
            for (int k = 0; k < t2->nb_entrees; k++) {
                result->tableaux_REL[idx_REL].entrees[k] = t2->entrees[k];
                corriger_entree_REL(&result->tableaux_REL[idx_REL].entrees[k],
                                   ctx2, 0, fusion, fusion_count, sym_map, table_sym_fusion);
            }
            
            idx_REL++;
        }
    }
    
    // Même logique pour RELA
    for (int i = 0; i < tables1->nb_RELA; i++) {
        elf_tableau_RELA *t1 = &tables1->tableaux_RELA[i];
        elf_tableau_RELA *match = NULL;
        
        for (int j = 0; j < tables2->nb_RELA; j++) {
            if (strcmp(t1->nom, tables2->tableaux_RELA[j].nom) == 0) {
                match = &tables2->tableaux_RELA[j];
                break;
            }
        }
        
        if (match) {
            result->tableaux_RELA[idx_RELA].nom = strdup(t1->nom);
            result->tableaux_RELA[idx_RELA].nb_entrees = t1->nb_entrees + match->nb_entrees;
            result->tableaux_RELA[idx_RELA].entrees = malloc(
                sizeof(elf_entree_RELA) * result->tableaux_RELA[idx_RELA].nb_entrees
            );
            
            for (int k = 0; k < t1->nb_entrees; k++) {
                result->tableaux_RELA[idx_RELA].entrees[k] = t1->entrees[k];
                corriger_entree_RELA(&result->tableaux_RELA[idx_RELA].entrees[k],
                                    ctx1, 1, fusion, fusion_count, sym_map, table_sym_fusion);
            }
            
            for (int k = 0; k < match->nb_entrees; k++) {
                result->tableaux_RELA[idx_RELA].entrees[t1->nb_entrees + k] = match->entrees[k];
                corriger_entree_RELA(&result->tableaux_RELA[idx_RELA].entrees[t1->nb_entrees + k],
                                    ctx2, 0, fusion, fusion_count, sym_map, table_sym_fusion);
            }
            
            idx_RELA++;
        } else {
            result->tableaux_RELA[idx_RELA].nom = strdup(t1->nom);
            result->tableaux_RELA[idx_RELA].nb_entrees = t1->nb_entrees;
            result->tableaux_RELA[idx_RELA].entrees = malloc(sizeof(elf_entree_RELA) * t1->nb_entrees);
            
            for (int k = 0; k < t1->nb_entrees; k++) {
                result->tableaux_RELA[idx_RELA].entrees[k] = t1->entrees[k];
                corriger_entree_RELA(&result->tableaux_RELA[idx_RELA].entrees[k],
                                    ctx1, 1, fusion, fusion_count, sym_map, table_sym_fusion);
            }
            
            idx_RELA++;
        }
    }
    
    for (int i = 0; i < tables2->nb_RELA; i++) {
        elf_tableau_RELA *t2 = &tables2->tableaux_RELA[i];
        int deja_traite = 0;
        
        for (int j = 0; j < tables1->nb_RELA; j++) {
            if (strcmp(t2->nom, tables1->tableaux_RELA[j].nom) == 0) {
                deja_traite = 1;
                break;
            }
        }
        
        if (!deja_traite) {
            result->tableaux_RELA[idx_RELA].nom = strdup(t2->nom);
            result->tableaux_RELA[idx_RELA].nb_entrees = t2->nb_entrees;
            result->tableaux_RELA[idx_RELA].entrees = malloc(sizeof(elf_entree_RELA) * t2->nb_entrees);
            
            for (int k = 0; k < t2->nb_entrees; k++) {
                result->tableaux_RELA[idx_RELA].entrees[k] = t2->entrees[k];
                corriger_entree_RELA(&result->tableaux_RELA[idx_RELA].entrees[k],
                                    ctx2, 0, fusion, fusion_count, sym_map, table_sym_fusion);
            }
            
            idx_RELA++;
        }
    }
    
    liberer_mapping_symboles(sym_map);
    return result;
}
