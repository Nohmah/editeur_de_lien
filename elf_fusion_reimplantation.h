#ifndef ELF_FUSION_REIMPLANTATION_H
#define ELF_FUSION_REIMPLANTATION_H

#include "elf_symboles.h"
#include "elf_reimplantation.h"
#include "elf_fusion_symboles.h"
#include "fusion_table.h"

/**
 * Fusionne deux tables de réimplantations et corrige les entrées
 * 
 * @param ctx1 Contexte du premier fichier ELF
 * @param ctx2 Contexte du second fichier ELF
 * @param tables1 Tables de réimplantations du premier fichier
 * @param tables2 Tables de réimplantations du second fichier
 * @param fusion Tableau de fusion des sections (avec offsets)
 * @param fusion_count Nombre d'éléments dans le tableau de fusion
 * @param table_sym_fusion Table des symboles fusionnée
 * @return Nouvelles tables de réimplantations fusionnées, ou NULL en cas d'erreur
 */
elf_tables_reimplantations* fusionner_tables_reimplantations(
    elf_contexte_t *ctx1, 
    elf_contexte_t *ctx2,
    elf_tables_reimplantations *tables1,
    elf_tables_reimplantations *tables2,
    section_fusion_t *fusion,
    int fusion_count,
    elf_table_sym_t *table_sym_fusion
);

#endif // ELF_FUSION_REIMPLANTATION_H