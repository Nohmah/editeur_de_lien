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
