#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <elf.h>
#include "util.h"
#include "elf_lecture_section.h"

int elf_lecture_section(FILE* f, char *section) {
    rewind(f);
    Elf32_Ehdr hdr;
    fread(&hdr, 1, sizeof(Elf32_Ehdr), f);
    
    uint32_t shoff = reverse_4(hdr.e_shoff); // offset de la table des sections
    uint16_t shnum = reverse_2(hdr.e_shnum); // nombre de sections
    uint16_t shstrndx = reverse_2(hdr.e_shstrndx); // index de la section qui contient les noms
    
    Elf32_Shdr *sections = malloc(shnum * sizeof(Elf32_Shdr));
    fseek(f, shoff, SEEK_SET);
    fread(sections, sizeof(Elf32_Shdr), shnum, f);
    
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
            free(sections);
            return 1;
        }
    } else {
        // C'est un nom
        uint32_t off  = reverse_4(sections[shstrndx].sh_offset);
    	uint32_t size = reverse_4(sections[shstrndx].sh_size);
        
        fseek(f, off, SEEK_SET);
        char *shstrtab = malloc(size);
        fread(shstrtab, 1, size, f);
        
        for (int i = 0; i < shnum; i++) {
            uint32_t name_offset = reverse_4(sections[i].sh_name);
            if (name_offset < size) {
                if (strcmp(shstrtab + name_offset, section) == 0){
                    section_index = i;
                    break;
                }
            }
        }
        free(shstrtab);
        if (section_index == -1) {
            fprintf(stderr, "Section '%s' non trouvée\n", section);
            free(sections);
            return 1;
        }
    }
    
    // Infos de la section trouvée
    Elf32_Shdr target = sections[section_index];
    uint32_t offset = reverse_4(target.sh_offset);
    uint32_t size = reverse_4(target.sh_size);
    
    // Obtenir le nom réel de la section
    char section_name[256] = "";
    if (shstrndx < shnum) {
        uint32_t shstrtab_offset = reverse_4(sections[shstrndx].sh_offset);
        uint32_t shstrtab_size = reverse_4(sections[shstrndx].sh_size);
        uint32_t name_offset = reverse_4(target.sh_name);
        
        if (name_offset < shstrtab_size) {
            fseek(f, shstrtab_offset + name_offset, SEEK_SET);
            // Lire jusqu'au premier null
            for (int i = 0; i < sizeof(section_name) - 1; i++) {
                char c;
                if (fread(&c, 1, 1, f) != 1 || c == '\0') break;
                section_name[i] = c;
            }
        }
    }
    
    // Afficher l'en-tête comme readelf
    const char *display_name = (strlen(section_name) > 0) ? section_name : section;
    printf("\nContenu hexadecimal de la section '%s':\n", display_name);
    
    // Vérifier si la section n'est pas vide
    if (size == 0) {
        printf("  Section vide\n");
        free(sections);
        return 0;
    }
    
    // Lire le contenu de la section
    fseek(f, offset, SEEK_SET);
    unsigned char *buffer = malloc(size);
    fread(buffer, 1, size, f);
    
    // Affichage hexdump (exactement comme readelf)
    for (uint32_t i = 0; i < size; i += 16) {
        printf("  0x%08x ", i);  // Format: espace, 8 chiffres hexa, espace
        
        // 16 octets hexa par ligne
        for (int j = 0; j < 16; j++) {
            if (j == 4 || j == 8 || j == 12) printf(" ");  // Espace au milieu (après 8 octets)
            
            if (i + j < size) {
                printf("%02x", buffer[i + j]);
            } else {
                printf("   ");  // Remplissage pour alignement
            }
        }
        
        printf("  ");  // 2 espaces avant la partie ASCII
        
        for (int j = 0; j < 16 && i + j < size; j++) {
            unsigned char c = buffer[i + j];
            printf("%c", (c >= 32 && c < 127) ? c : '.');
        }
        printf("\n");
    }
    
    // Nettoyage
    free(buffer);
    free(sections);
    
    return 0;
}