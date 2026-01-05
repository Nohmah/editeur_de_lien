#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <elf.h>
#include "util.h"
#include "elf_header.h"
#include "elf_symboles.h"

void afficher_entete(elf_contexte_t *ctx) {
    
    printf("ELF Header:\n");
    printf("Magic:   ");
    for(int i = 0; i < EI_NIDENT; i++) {
        printf("%02x ", ctx->ehdr.e_ident[i]);
    }
    printf("\n");

    printf("Class:   %s\n", (ctx->ehdr.e_ident[EI_CLASS] == ELFCLASS32 ? "ELF32" : "Inconnu"));
    printf("Data:   %s\n", (ctx->ehdr.e_ident[EI_DATA] == ELFDATA2MSB ? "Big Endian" : "Inconnu"));
    printf("Version:   %d\n", ctx->ehdr.e_ident[EI_VERSION]);
    printf("OS/ABI:   %d\n", ctx->ehdr.e_ident[EI_OSABI]);
    printf("ABI Version:   %d\n", ctx->ehdr.e_ident[EI_ABIVERSION]);

    printf("Type:   %s\n", (ctx->ehdr.e_type == ET_REL ? "REL (Fichier de réadressage)" : "Autres"));
    printf("Machine:   %s\n", (ctx->ehdr.e_machine == EM_ARM ? "ARM" : "Inconnu"));
    printf("Version:   0x%08x\n", ctx->ehdr.e_version);
    printf("Entry point address:   0x%08x\n", ctx->ehdr.e_entry);

    printf("Start of program headers:   %u %s\n", ctx->ehdr.e_phoff, "(octets dans le fichier)");
    printf("Start of section headers:   %u %s\n", ctx->ehdr.e_shoff, "(octets dans le fichier)");

    printf("Flags:   0x%08x\n", ctx->ehdr.e_flags);

    printf("Size of this header: %u %s\n", ctx->ehdr.e_ehsize, "(octets)");
    printf("Size of program headers:   %u %s\n", ctx->ehdr.e_phentsize, "(octets)");
    printf("Number of program headers:   %u\n", ctx->ehdr.e_phnum);
    printf("Size of section headers:   %u %s\n", ctx->ehdr.e_shentsize, "(octets)");
    printf("Number of section headers:   %u\n", ctx->ehdr.e_shnum);
    printf("Section header string table index:   %u\n", ctx->ehdr.e_shstrndx);
}

int verifier_fichier(FILE* f, elf_contexte_t *ctx) {
    if (!ctx) return 0;
    ctx->file = f;
    
    if (lire_entete_elf(ctx) != 0) {
        fprintf(stderr, "Erreur lors de la lecture de l'en-tête ELF\n");
        return 0;
    }
    
    // Vérification de la signature ELF (magic number)
    if (ctx->ehdr.e_ident[EI_MAG0] != ELFMAG0 ||
        ctx->ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
        ctx->ehdr.e_ident[EI_MAG2] != ELFMAG2 ||
        ctx->ehdr.e_ident[EI_MAG3] != ELFMAG3) {
        fprintf(stderr, "Pas un fichier ELF.\n");
        return 0;
    }

    // Vérification de la classe (32 bits)
    if (ctx->ehdr.e_ident[EI_CLASS] != ELFCLASS32) {
        fprintf(stderr, "ELF pas en 32 bits\n");
        return 0;
    }

    // Vérification de l'endianness (big endian)
    if (ctx->ehdr.e_ident[EI_DATA] != ELFDATA2MSB) {
        fprintf(stderr, "ELF pas en big endian\n");
        return 0;
    }

    // Vérification de l'architecture (ARM)
    if (ctx->ehdr.e_machine != EM_ARM) {
        fprintf(stderr, "Pas ARM.\n");
        return 0;
    }

    return 1;
}