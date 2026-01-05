#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <elf.h>
#include "util.h"
#include "elf_lecture_section.h"
#include "elf_symboles.h"

int elf_lecture_section(elf_contexte_t *ctx, const char *section) {
    
    FILE *f = ctx->file;
    uint16_t shnum = ctx->ehdr.e_shnum;

    // Trouver l'index de la section demandée
    int section_index = -1;
    char *end;
    long num = strtol(section, &end, 10);
    
    if (*end == '\0') {
        // C'est un numéro
        if (num >= 0 && num < shnum) {
            section_index = (int)num;
        } else {
            fprintf(stderr, "Numéro de section invalide. Candidats:(0-%d)\n", shnum-1);
            return 1;
        }
    } else {
        // C'est un nom - utiliser trouver_section() de elf_symboles.c
        section_index = trouver_section(ctx, section);
        if (section_index == -1) {
            fprintf(stderr, "Section '%s' non trouvée\n", section);
            return 1;
        }
    }
    
    // Infos sur la section trouvée
    Elf32_Shdr target = ctx->sections[section_index];
    uint32_t offset = target.sh_offset;
    uint32_t size = target.sh_size;
    
    // Récupérer le nom de la section
    const char *section_name = ctx->noms_sections + target.sh_name;
    
    // Affichage de l'en-tête
    printf("\nContenu hexadecimal de la section '%s':\n", section_name);
    
    if (size == 0) {
        printf("Section vide\n");
        return 0;
    }
    
    // Lecture du contenu
    fseek(f, offset, SEEK_SET);
    unsigned char *buffer = malloc(size);
    if (!buffer) {
        perror("Erreur d'allocation mémoire");
        return 1;
    }
    
    if (fread(buffer, 1, size, f) != size) {
        fprintf(stderr, "Erreur lors de la lecture du contenu de la section\n");
        free(buffer);
        return 1;
    }
    
    // Écriture du contenu
    for (uint32_t i = 0; i < size; i += 16) {
        printf("  0x%08x ", i);
        
        for (int j = 0; j < 16; j++) {
            if (j == 4 || j == 8 || j == 12) printf(" ");
            
            if (i + j < size) {
                printf("%02x", buffer[i + j]);
            } else {
                printf("  ");
            }
        }
        
        printf(" ");  // 2 espaces avant la partie ASCII
        
        for (int j = 0; j < 16 && i + j < size; j++) {
            unsigned char c = buffer[i + j];
            printf("%c", (c >= 32 && c < 127) ? c : '.');
        }
        printf("\n");
    }

    free(buffer);
    
    return 0;
}