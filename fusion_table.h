#ifndef __FUSION_TABLE__
#define __FUSION_TABLE__

#include <elf.h>
#include <stdio.h>
#include "elf_symboles.h"

typedef struct {
    char* name;
    Elf32_Shdr shdr_out;

    int idx_ctx1;
    int idx_ctx2;
    int idx_out;

    uint32_t size1;
    uint32_t size2;

    uint32_t offset;
} section_fusion_t;

int fusion_section_table(elf_contexte_t* ctx1, elf_contexte_t* ctx2, section_fusion_t** sections, int* nb_sections);
void concat_offset(elf_contexte_t* ctx1, elf_contexte_t* ctx2, section_fusion_t* table, int count);
void build_output_section_headers(elf_contexte_t *ctx1, elf_contexte_t *ctx2, elf_contexte_t *ctxout, section_fusion_t *fusion, int fusion_count);
int write_sections(elf_contexte_t *ctx1, elf_contexte_t *ctx2, elf_contexte_t *ctxout, section_fusion_t *fusion, int fusion_count);

#endif