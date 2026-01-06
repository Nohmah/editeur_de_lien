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
    
    // Vérifier que les tables de noms existent
    if (!ctx1->noms_symboles || !ctx2->noms_symboles || !table_fusion->noms_symboles) {
        fprintf(stderr, "ERREUR: noms_symboles NULL - ctx1=%p ctx2=%p table_fusion=%p\n",
                (void*)ctx1->noms_symboles, (void*)ctx2->noms_symboles, (void*)table_fusion->noms_symboles);
        free(mapping);
        return NULL;
    }
    
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
    
    // Stratégie simple: mapper par position relative dans les locaux/globaux
    // Tous les locaux de ctx1 sont au début, puis locaux de ctx2, puis globaux
    
    int nb_locaux_ctx1 = 0;
    int nb_globaux_ctx1 = 0;
    
    // Compter locaux et globaux de ctx1
    for (int i = 0; i < ctx1->nb_symboles; i++) {
        if (ELF32_ST_BIND(ctx1->symboles[i].st_info) == STB_LOCAL) {
            nb_locaux_ctx1++;
        } else {
            nb_globaux_ctx1++;
        }
    }
    
    // Mapper les symboles de ctx1 (ils gardent leur position relative)
    int idx_local_ctx1 = 0;
    int idx_global_fusion = nb_locaux_ctx1;
    
    for (int i = 0; i < ctx1->nb_symboles; i++) {
        const char *nom1 = (ctx1->symboles[i].st_name != 0 && ctx1->noms_symboles) ? 
                           ctx1->noms_symboles + ctx1->symboles[i].st_name : "";
        
        if (ELF32_ST_BIND(ctx1->symboles[i].st_info) == STB_LOCAL) {
            mapping->map_ctx1[i] = idx_local_ctx1;
            idx_local_ctx1++;
        } else {
            // Chercher parmi les globaux de la table fusionnée
            int found = 0;
            for (int j = 0; j < table_fusion->nb_symboles; j++) {
                if (ELF32_ST_BIND(table_fusion->symboles[j].st_info) != STB_LOCAL) {
                    const char *nom_fusion = (table_fusion->symboles[j].st_name != 0 && table_fusion->noms_symboles) ?
                                             table_fusion->noms_symboles + table_fusion->symboles[j].st_name : "";
                    
                    if (nom1 && nom_fusion && strcmp(nom1, nom_fusion) == 0) {
                        mapping->map_ctx1[i] = j;
                        found = 1;
                        break;
                    }
                }
            }
            if (!found) {
                mapping->map_ctx1[i] = idx_global_fusion;
                idx_global_fusion++;
            }
        }
    }
    
    // Mapper les symboles de ctx2
    int nb_locaux_ctx2 = 0;
    for (int i = 0; i < ctx2->nb_symboles; i++) {
        if (ELF32_ST_BIND(ctx2->symboles[i].st_info) == STB_LOCAL) {
            nb_locaux_ctx2++;
        }
    }
    
    int idx_local_ctx2 = nb_locaux_ctx1;  // Les locaux de ctx2 commencent après ceux de ctx1
    
    for (int i = 0; i < ctx2->nb_symboles; i++) {
        const char *nom2 = (ctx2->symboles[i].st_name != 0 && ctx2->noms_symboles) ?
                           ctx2->noms_symboles + ctx2->symboles[i].st_name : "";
        
        if (ELF32_ST_BIND(ctx2->symboles[i].st_info) == STB_LOCAL) {
            mapping->map_ctx2[i] = idx_local_ctx2;
            idx_local_ctx2++;
        } else {
            // Chercher parmi les globaux de la table fusionnée
            int found = 0;
            for (int j = 0; j < table_fusion->nb_symboles; j++) {
                if (ELF32_ST_BIND(table_fusion->symboles[j].st_info) != STB_LOCAL) {
                    const char *nom_fusion = (table_fusion->symboles[j].st_name != 0 && table_fusion->noms_symboles) ?
                                             table_fusion->noms_symboles + table_fusion->symboles[j].st_name : "";
                    
                    if (nom2 && nom_fusion && strcmp(nom2, nom_fusion) == 0) {
                        mapping->map_ctx2[i] = j;
                        found = 1;
                        break;
                    }
                }
            }
            if (!found) {
                mapping->map_ctx2[i] = idx_global_fusion;
                idx_global_fusion++;
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

// Trouve l'offset de concaténation pour une section donnée
uint32_t trouver_offset_concat(section_fusion_t *fusion, int fusion_count, int idx_section, int from_ctx1) {
    for (int i = 0; i < fusion_count; i++) {
        if (from_ctx1) {
            if (fusion[i].idx_ctx1 == idx_section) {
                return 0; // Les sections de ctx1 ne sont pas décalées
            }
        } else {
            if (fusion[i].idx_ctx2 == idx_section) {
                return fusion[i].offset;
            }
        }
    }
    return 0;
}

// Trouve la section associée à une table de relocation
int trouver_section_reliee(elf_contexte_t *ctx, const char *nom_table_rel) {
    // Le nom de la table est .rel.XXXX ou .rela.XXXX
    // On cherche la section XXXX
    const char *nom_section = NULL;
    
    if (strncmp(nom_table_rel, ".rel.", 5) == 0) {
        nom_section = nom_table_rel + 4;  // Sauter ".rel"
    } else if (strncmp(nom_table_rel, ".rela.", 6) == 0) {
        nom_section = nom_table_rel + 5;  // Sauter ".rela"
    }
    
    if (!nom_section) return -1;
    
    return trouver_section(ctx, nom_section);
}
// Corrige une entrée de relocation REL
void corriger_entree_REL(elf_entree_REL *entree, 
                         elf_contexte_t *ctx_source,
                         int from_ctx1,
                         const char *nom_table_rel,
                         section_fusion_t *fusion, int fusion_count,
                         symbol_mapping_t *sym_map, elf_table_sym_t *table_sym_fusion) {
    
    int old_sym_idx = ELF32_R_SYM(entree->info);
    
    // 1. Trouver la section concernée par cette table de relocation
    int section_idx = trouver_section_reliee(ctx_source, nom_table_rel);
    
    if (section_idx >= 0) {
        // Ajouter l'offset de concaténation pour ctx2
        uint32_t offset_concat = trouver_offset_concat(fusion, fusion_count, section_idx, from_ctx1);
        entree->offset += offset_concat;
    }
    
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
    
    if (new_sym_idx >= 0 && new_sym_idx < table_sym_fusion->nb_symboles) {
        unsigned char type = ELF32_R_TYPE(entree->info);
        entree->info = ELF32_R_INFO(new_sym_idx, type);
        
        // Mettre à jour le nom et la valeur
        if (entree->nom) free(entree->nom);
        if (table_sym_fusion->symboles[new_sym_idx].st_name != 0) {
            const char *nom_sym = table_sym_fusion->noms_symboles + table_sym_fusion->symboles[new_sym_idx].st_name;
            entree->nom = strdup(nom_sym);
        } else {
            entree->nom = strdup("");
        }
        entree->valeur = table_sym_fusion->symboles[new_sym_idx].st_value;
    }
}

// Corrige une entrée de relocation RELA
void corriger_entree_RELA(elf_entree_RELA *entree, 
                          elf_contexte_t *ctx_source,
                          int from_ctx1,
                          const char *nom_table_rel,
                          section_fusion_t *fusion, int fusion_count,
                          symbol_mapping_t *sym_map, 
                          elf_table_sym_t *table_sym_fusion) {
    
    int old_sym_idx = ELF32_R_SYM(entree->info);
    
    // 1. Trouver la section concernée
    int section_idx = trouver_section_reliee(ctx_source, nom_table_rel);
    
    if (section_idx >= 0) {
        uint32_t offset_concat = trouver_offset_concat(fusion, fusion_count, section_idx, from_ctx1);
        entree->offset += offset_concat;
    }
    
    // Récupérer l'ancien symbole AVANT de modifier l'index
    Elf32_Sym old_sym;
    memset(&old_sym, 0, sizeof(old_sym));
    if (old_sym_idx >= 0 && old_sym_idx < ctx_source->nb_symboles) {
        old_sym = ctx_source->symboles[old_sym_idx];
    }
    
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
    
    if (new_sym_idx >= 0 && new_sym_idx < table_sym_fusion->nb_symboles) {
        unsigned char type = ELF32_R_TYPE(entree->info);
        entree->info = ELF32_R_INFO(new_sym_idx, type);
        
        // 3. Corriger l'addend si c'est un symbole SECTION
        if (old_sym_idx >= 0 && ELF32_ST_TYPE(old_sym.st_info) == STT_SECTION) {
            uint32_t section_offset_concat = trouver_offset_concat(fusion, fusion_count,
                                                                    old_sym.st_shndx, from_ctx1);
            
            // Pour R_ARM_JUMP24 et R_ARM_CALL, addend est en instructions (divisé par 4)
            if (type == R_ARM_JUMP24 || type == R_ARM_CALL || type == 1) { // R_ARM_PC24
                entree->addend += section_offset_concat / 4;
            } else {
                entree->addend += section_offset_concat;
            }
        }
        
        // Mettre à jour le nom et la valeur
        if (entree->nom) free(entree->nom);
        if (table_sym_fusion->symboles[new_sym_idx].st_name != 0) {
            const char *nom_sym = table_sym_fusion->noms_symboles + table_sym_fusion->symboles[new_sym_idx].st_name;
            entree->nom = strdup(nom_sym);
        } else {
            entree->nom = strdup("");
        }
        entree->valeur = table_sym_fusion->symboles[new_sym_idx].st_value;
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
    
    if (!ctx1 || !ctx2 || !tables1 || !tables2 || !fusion || !table_sym_fusion) {
        fprintf(stderr, "Erreur: paramètres NULL dans fusionner_tables_reimplantations\n");
        return NULL;
    }
    
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
    
    // Compter les tables totales (on fusionne celles de même nom)
    int total_REL = tables1->nb_REL;
    for (int i = 0; i < tables2->nb_REL; i++) {
        int found = 0;
        for (int j = 0; j < tables1->nb_REL; j++) {
            if (strcmp(tables2->tableaux_REL[i].nom, tables1->tableaux_REL[j].nom) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) total_REL++;
    }
    
    int total_RELA = tables1->nb_RELA;
    for (int i = 0; i < tables2->nb_RELA; i++) {
        int found = 0;
        for (int j = 0; j < tables1->nb_RELA; j++) {
            if (strcmp(tables2->tableaux_RELA[i].nom, tables1->tableaux_RELA[j].nom) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) total_RELA++;
    }
    
    result->nb_REL = total_REL;
    result->nb_RELA = total_RELA;
    result->tableaux_REL = calloc(total_REL, sizeof(elf_tableau_REL));
    result->tableaux_RELA = calloc(total_RELA, sizeof(elf_tableau_RELA));
    
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
            result->tableaux_REL[idx_REL].entrees = calloc(
                result->tableaux_REL[idx_REL].nb_entrees, sizeof(elf_entree_REL)
            );
            
            // Copier et corriger les entrées de t1
            for (int k = 0; k < t1->nb_entrees; k++) {
                result->tableaux_REL[idx_REL].entrees[k] = t1->entrees[k];
                result->tableaux_REL[idx_REL].entrees[k].type = t1->entrees[k].type ? strdup(t1->entrees[k].type) : strdup("");
                result->tableaux_REL[idx_REL].entrees[k].nom = t1->entrees[k].nom ? strdup(t1->entrees[k].nom) : strdup("");
                corriger_entree_REL(&result->tableaux_REL[idx_REL].entrees[k],
                                   ctx1, 1, t1->nom, fusion, fusion_count, sym_map, table_sym_fusion);
            }
            
            // Copier et corriger les entrées de match
            for (int k = 0; k < match->nb_entrees; k++) {
                result->tableaux_REL[idx_REL].entrees[t1->nb_entrees + k] = match->entrees[k];
                result->tableaux_REL[idx_REL].entrees[t1->nb_entrees + k].type = match->entrees[k].type ? strdup(match->entrees[k].type) : strdup("");
                result->tableaux_REL[idx_REL].entrees[t1->nb_entrees + k].nom = match->entrees[k].nom ? strdup(match->entrees[k].nom) : strdup("");
                corriger_entree_REL(&result->tableaux_REL[idx_REL].entrees[t1->nb_entrees + k],
                                   ctx2, 0, match->nom, fusion, fusion_count, sym_map, table_sym_fusion);
            }
            
            idx_REL++;
        } else {
            // Juste copier t1
            result->tableaux_REL[idx_REL].nom = strdup(t1->nom);
            result->tableaux_REL[idx_REL].nb_entrees = t1->nb_entrees;
            result->tableaux_REL[idx_REL].entrees = calloc(t1->nb_entrees, sizeof(elf_entree_REL));
            
            for (int k = 0; k < t1->nb_entrees; k++) {
                result->tableaux_REL[idx_REL].entrees[k] = t1->entrees[k];
                result->tableaux_REL[idx_REL].entrees[k].type = t1->entrees[k].type ? strdup(t1->entrees[k].type) : strdup("");
                result->tableaux_REL[idx_REL].entrees[k].nom = t1->entrees[k].nom ? strdup(t1->entrees[k].nom) : strdup("");
                corriger_entree_REL(&result->tableaux_REL[idx_REL].entrees[k],
                                   ctx1, 1, t1->nom, fusion, fusion_count, sym_map, table_sym_fusion);
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
            result->tableaux_REL[idx_REL].entrees = calloc(t2->nb_entrees, sizeof(elf_entree_REL));
            
            for (int k = 0; k < t2->nb_entrees; k++) {
                result->tableaux_REL[idx_REL].entrees[k] = t2->entrees[k];
                result->tableaux_REL[idx_REL].entrees[k].type = t2->entrees[k].type ? strdup(t2->entrees[k].type) : strdup("");
                result->tableaux_REL[idx_REL].entrees[k].nom = t2->entrees[k].nom ? strdup(t2->entrees[k].nom) : strdup("");
                corriger_entree_REL(&result->tableaux_REL[idx_REL].entrees[k],
                                   ctx2, 0, t2->nom, fusion, fusion_count, sym_map, table_sym_fusion);
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
            result->tableaux_RELA[idx_RELA].entrees = calloc(
                result->tableaux_RELA[idx_RELA].nb_entrees, sizeof(elf_entree_RELA)
            );
            
            for (int k = 0; k < t1->nb_entrees; k++) {
                result->tableaux_RELA[idx_RELA].entrees[k] = t1->entrees[k];
                result->tableaux_RELA[idx_RELA].entrees[k].type = t1->entrees[k].type ? strdup(t1->entrees[k].type) : strdup("");
                result->tableaux_RELA[idx_RELA].entrees[k].nom = t1->entrees[k].nom ? strdup(t1->entrees[k].nom) : strdup("");
                corriger_entree_RELA(&result->tableaux_RELA[idx_RELA].entrees[k],
                                    ctx1, 1, t1->nom, fusion, fusion_count, sym_map, table_sym_fusion);
            }
            
            for (int k = 0; k < match->nb_entrees; k++) {
                result->tableaux_RELA[idx_RELA].entrees[t1->nb_entrees + k] = match->entrees[k];
                result->tableaux_RELA[idx_RELA].entrees[t1->nb_entrees + k].type = match->entrees[k].type ? strdup(match->entrees[k].type) : strdup("");
                result->tableaux_RELA[idx_RELA].entrees[t1->nb_entrees + k].nom = match->entrees[k].nom ? strdup(match->entrees[k].nom) : strdup("");
                corriger_entree_RELA(&result->tableaux_RELA[idx_RELA].entrees[t1->nb_entrees + k],
                                    ctx2, 0, match->nom, fusion, fusion_count, sym_map, table_sym_fusion);
            }
            
            idx_RELA++;
        } else {
            result->tableaux_RELA[idx_RELA].nom = strdup(t1->nom);
            result->tableaux_RELA[idx_RELA].nb_entrees = t1->nb_entrees;
            result->tableaux_RELA[idx_RELA].entrees = calloc(t1->nb_entrees, sizeof(elf_entree_RELA));
            
            for (int k = 0; k < t1->nb_entrees; k++) {
                result->tableaux_RELA[idx_RELA].entrees[k] = t1->entrees[k];
                result->tableaux_RELA[idx_RELA].entrees[k].type = t1->entrees[k].type ? strdup(t1->entrees[k].type) : strdup("");
                result->tableaux_RELA[idx_RELA].entrees[k].nom = t1->entrees[k].nom ? strdup(t1->entrees[k].nom) : strdup("");
                corriger_entree_RELA(&result->tableaux_RELA[idx_RELA].entrees[k],
                                    ctx1, 1, t1->nom, fusion, fusion_count, sym_map, table_sym_fusion);
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
            result->tableaux_RELA[idx_RELA].entrees = calloc(t2->nb_entrees, sizeof(elf_entree_RELA));
            
            for (int k = 0; k < t2->nb_entrees; k++) {
                result->tableaux_RELA[idx_RELA].entrees[k] = t2->entrees[k];
                result->tableaux_RELA[idx_RELA].entrees[k].type = t2->entrees[k].type ? strdup(t2->entrees[k].type) : strdup("");
                result->tableaux_RELA[idx_RELA].entrees[k].nom = t2->entrees[k].nom ? strdup(t2->entrees[k].nom) : strdup("");
                corriger_entree_RELA(&result->tableaux_RELA[idx_RELA].entrees[k],
                                    ctx2, 0, t2->nom, fusion, fusion_count, sym_map, table_sym_fusion);
            }
            
            idx_RELA++;
        }
    }
    
    liberer_mapping_symboles(sym_map);
    return result;
}