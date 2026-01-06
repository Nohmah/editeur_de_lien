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

