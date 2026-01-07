#include <stdio.h>
#include <elf.h>
#include <stdlib.h>
#include <string.h>

#include "fusion_table.h"

static const char* section_name(const elf_contexte_t* ctx, int i){
    return ctx->noms_sections + ctx->sections[i].sh_name;
}

// Vérifie si une section doit être ignorée
static int should_skip_section(const char* name) {
    // Ignorer les sections de debug
    if (strncmp(name, ".debug", 6) == 0)
        return 1;
    
    // Ignorer la section comment
    if (strcmp(name, ".comment") == 0)
        return 1;
    
    // Ignorer les sections ARM-specific qui ne doivent pas être fusionnées
    if (strcmp(name, ".ARM.attributes") == 0)
        return 1;
    
    return 0;
}

int fusion_section_table(elf_contexte_t* ctx1, elf_contexte_t* ctx2, section_fusion_t** sections, int* nb_sections){
    *sections = NULL;
    *nb_sections = 0;

    // Sections PROGBITS du fichier 1
    for (int i = 0; i < ctx1->ehdr.e_shnum; i++) {
        if (ctx1->sections[i].sh_type != SHT_PROGBITS)
            continue;

        const char *name = section_name(ctx1, i);

        // Ignorer debug, comment, etc.
        if (should_skip_section(name))
            continue;

        section_fusion_t *tmp = realloc(
            *sections,
            (*nb_sections + 1) * sizeof(**sections)
        );
        if (!tmp) return -1;

        *sections = tmp;

        (*sections)[*nb_sections].name = strdup(name);
        (*sections)[*nb_sections].idx_ctx1 = i;
        (*sections)[*nb_sections].idx_ctx2 = -1;
        (*sections)[*nb_sections].idx_out  = -1; // initialisé
        (*sections)[*nb_sections].size1 = 0;
        (*sections)[*nb_sections].size2 = 0;
        (*sections)[*nb_sections].offset = 0;

        (*nb_sections)++;
    }

    // Sections PROGBITS du fichier 2
    for (int i = 0; i < ctx2->ehdr.e_shnum; i++) {
        if (ctx2->sections[i].sh_type != SHT_PROGBITS)
            continue;

        const char *name = section_name(ctx2, i);
        
        // Ignorer debug, comment, etc.
        if (should_skip_section(name))
            continue;

        int found = 0;

        for (int j = 0; j < *nb_sections; j++) {
            if (strcmp((*sections)[j].name, name) == 0) {
                (*sections)[j].idx_ctx2 = i;
                found = 1;
                break;
            }
        }

        if (!found) {
            section_fusion_t *tmp = realloc(
                *sections,
                (*nb_sections + 1) * sizeof(**sections)
            );
            if (!tmp) return -1;

            *sections = tmp;

            (*sections)[*nb_sections].name = strdup(name);
            (*sections)[*nb_sections].idx_ctx1 = -1;
            (*sections)[*nb_sections].idx_ctx2 = i;
            (*sections)[*nb_sections].idx_out  = -1;
            (*sections)[*nb_sections].size1 = 0;
            (*sections)[*nb_sections].size2 = 0;
            (*sections)[*nb_sections].offset = 0;

            (*nb_sections)++;
        }
    }

    return 0;
}

void concat_offset(elf_contexte_t* ctx1, elf_contexte_t* ctx2, section_fusion_t* table, int count){
    for (int i = 0; i < count; i++) {
        section_fusion_t* s = &table[i];

        s->size1 = (s->idx_ctx1 != -1) ? ctx1->sections[s->idx_ctx1].sh_size : 0;
        s->size2 = (s->idx_ctx2 != -1) ? ctx2->sections[s->idx_ctx2].sh_size : 0;
        s->offset = (s->idx_ctx1 != -1 && s->idx_ctx2 != -1) ? s->size1 : 0;
    }
}

void build_output_section_headers(elf_contexte_t *ctx1, elf_contexte_t *ctx2, elf_contexte_t *ctxout, section_fusion_t *fusion, int fusion_count) {
    ctxout->ehdr.e_shnum = fusion_count;
    ctxout->sections = calloc(fusion_count, sizeof(Elf32_Shdr));
    if (!ctxout->sections) {
        perror("calloc");
        exit(1);
    }

    int out_idx = 0;

    // Sections venant de ctx1
    for (int i = 0; i < fusion_count; i++) {
        section_fusion_t *s = &fusion[i];
        if (s->idx_ctx1 != -1) {
            ctxout->sections[out_idx] = ctx1->sections[s->idx_ctx1];

            if (s->idx_ctx2 != -1) {
                ctxout->sections[out_idx].sh_size = s->size1 + s->size2;
            }

            s->idx_out = out_idx; // mémorise l'index de sortie
            out_idx++;
        }
    }

    // Sections présentes uniquement dans ctx2
    for (int i = 0; i < fusion_count; i++) {
        section_fusion_t *s = &fusion[i];
        if (s->idx_ctx1 == -1 && s->idx_ctx2 != -1) {
            ctxout->sections[out_idx] = ctx2->sections[s->idx_ctx2];
            s->idx_out = out_idx;
            out_idx++;
        }
    }
}

int write_sections(elf_contexte_t *ctx1, elf_contexte_t *ctx2, elf_contexte_t *ctxout, section_fusion_t *fusion, int fusion_count) {
    if (!ctxout->file) return -1;

    // Écrit à la fin et définit sh_offset pour chaque section
    fseek(ctxout->file, 0, SEEK_END);

    for (int i = 0; i < fusion_count; i++) {
        section_fusion_t *s = &fusion[i];
        Elf32_Shdr *out = &ctxout->sections[s->idx_out];

        long pos = ftell(ctxout->file);
        if (pos < 0) return -1;
        out->sh_offset = (Elf32_Word)pos; // offset défini au moment de l'écriture

        // Écrit la partie ctx1
        if (s->idx_ctx1 != -1) {
            const Elf32_Shdr *sh1 = &ctx1->sections[s->idx_ctx1];
            if (sh1->sh_size) {
                void *buf = malloc(sh1->sh_size);
                if (!buf) return -1;
                fseek(ctx1->file, sh1->sh_offset, SEEK_SET);
                if (fread(buf, 1, sh1->sh_size, ctx1->file) != sh1->sh_size) { free(buf); return -1; }
                if (fwrite(buf, 1, sh1->sh_size, ctxout->file) != sh1->sh_size) { free(buf); return -1; }
                free(buf);
            }
        }
        // Puis la partie ctx2
        if (s->idx_ctx2 != -1) {
            const Elf32_Shdr *sh2 = &ctx2->sections[s->idx_ctx2];
            if (sh2->sh_size) {
                void *buf = malloc(sh2->sh_size);
                if (!buf) return -1;
                fseek(ctx2->file, sh2->sh_offset, SEEK_SET);
                if (fread(buf, 1, sh2->sh_size, ctx2->file) != sh2->sh_size) { free(buf); return -1; }
                if (fwrite(buf, 1, sh2->sh_size, ctxout->file) != sh2->sh_size) { free(buf); return -1; }
                free(buf);
            }
        }
    }
    return 0;
}