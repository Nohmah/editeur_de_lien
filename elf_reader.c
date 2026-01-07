#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "elf_header.h"
#include "elf_section_table.h"
#include "elf_symboles.h"
#include "elf_lecture_section.h"
#include "elf_reimplantation.h"
#include "fusion_table.h"
#include "fonc_inversion.h"
#include "elf_fusion_symboles.h"
#include "elf_fusion_reimplantation.h"

void usage(char *programme) {
    fprintf(stderr, "\nUsage:\n");
    fprintf(stderr, "%s <fichier ELF> <commande>\n\n", programme);
    fprintf(stderr, "Commandes disponibles :\n");
    fprintf(stderr, "  -h                 Afficher l'en-tête\n");
    fprintf(stderr, "  -S                 Afficher la table des sections\n");
    fprintf(stderr, "  -x <section>       Afficher le contenu d'une section\n");
    fprintf(stderr, "  -s                 Afficher la table des symboles\n");
    fprintf(stderr, "  -r                 Afficher les tables de réimplantation\n");
    fprintf(stderr, "  -f <fichier ELF2>  Fusionner avec un autre fichier ELF\n");
    fprintf(stderr, "  --help             Afficher ce message\n");
    fprintf(stderr, "Pour -x, spécifiez le nom ou le numéro de la section\n");
    fprintf(stderr, "pour la fusion le fichier fusionné sera 'fusion.o'\n");
    fprintf(stderr, "Seuls les fichiers ELF 32 bits ARM big endian sont supportés.\n\n");
}
int main(int argc, char *argv[]) {
	if (argc==2 && strcmp(argv[1], "--help")==0){
		usage(argv[0]);
		exit(0);
	}

	if (argc<3){
		fprintf(stderr, "Trop peu d'arguments donnés.\n");
		usage(argv[0]);
		exit(1);
	}

	// Vérification de si le fichier est exploitable ou non
	FILE* f = fopen(argv[1], "rb");
	if(!f){
		if (errno == ENOENT){
	        fprintf(stderr, "Erreur : le fichier '%s' n'existe pas.\n", argv[1]);
		} else {
			fprintf(stderr, "Erreur lors de l'ouverture du fichier.\n");
		}
		exit(1);
	}

	elf_contexte_t ctx;
	memset(&ctx, 0, sizeof(ctx));

	if (!verifier_fichier(f, &ctx)){
	    fprintf(stderr, "Ce programme ne supporte que les fichiers ELF 32 bits ARM big endian.\n");
	    fclose(f);
	    exit(1);
	}

	// Exécution de la commande demandée
	char *commande = argv[2];

	if (strcmp(commande, "-h") == 0) {
    	afficher_entete(&ctx);
	}
	else if (strcmp(commande, "-S") == 0) {
	    lire_table_sections(&ctx);
	    lire_noms_sections(&ctx);
	    afficher_sections_table(&ctx);
	}
	else if (strcmp(commande, "-x") == 0) {
		lire_table_sections(&ctx);
		lire_noms_sections(&ctx);
	    if (argc >= 4) {
	        elf_lecture_section(&ctx, argv[3]);
	    } else {
		    afficher_sections_table(&ctx);
	        printf("Veuillez entrer le nom ou le numéro de la section : ");
	        char section[100];
	        if (fgets(section, sizeof(section), stdin) != NULL) {
	            section[strcspn(section, "\n")] = '\0';
	            if (strlen(section) > 0) {
	                elf_lecture_section(&ctx, section);
	            } else {
	                fprintf(stderr, "Erreur: Aucune section spécifiée.\n");
	                exit(1);
	            }
	        }
	    }
	}
	else if (strcmp(commande, "-s") == 0) {
	    lire_table_sections(&ctx);
	    lire_noms_sections(&ctx);
	    lire_table_symboles(&ctx);
	}
	else if (strcmp(commande, "-r") == 0) {
	    lire_table_sections(&ctx);
	    lire_noms_sections(&ctx);
	    charger_table_symboles(&ctx);
      	elf_tables_reimplantations* tables = charger_tables_reimplantations(&ctx);
   		afficher_tables_reimplantations(tables);
    	liberer_tables_reimplantations(tables);
	}
	else if (strcmp(commande, "-f") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s <file1.o> -f <file2.o>\n", argv[0]);
            exit(1);
        }

        elf_contexte_t ctx2, out;
        memset(&ctx2, 0, sizeof(ctx2));
        memset(&out, 0, sizeof(out));

        FILE *f2 = fopen(argv[3], "rb");
        if (!f2) { perror(argv[3]); exit(1); }

        if (!verifier_fichier(f2, &ctx2)) { fclose(f2); exit(1); }

        // Charger sections + noms pour les deux fichiers
        if (lire_table_sections(&ctx) || lire_noms_sections(&ctx) ||
            lire_table_sections(&ctx2) || lire_noms_sections(&ctx2)) {
            fprintf(stderr, "Erreur de lecture des sections/noms\n");
            fclose(f2); exit(1);
        }

        section_fusion_t *fusion = NULL;
        int fusion_count = 0;

        if (fusion_section_table(&ctx, &ctx2, &fusion, &fusion_count) != 0) {
            fprintf(stderr, "fusion_section_table a échoué\n");
            fclose(f2); exit(1);
        }

        // Calcul des offsets
		concat_offset(&ctx, &ctx2, fusion, fusion_count);

        // Contexte de sortie
        elf_contexte_t out_ctx;
        memset(&out_ctx, 0, sizeof(out_ctx));
        out_ctx.file = fopen("fusion.o", "wb");
        if (!out_ctx.file) { perror("fusion.o"); exit(1); }

        // Construire les en-têtes des sections PROGBITS seulement
        build_output_section_headers(&ctx, &ctx2, &out_ctx, fusion, fusion_count);

        // Construire la .shstrtab (table des noms des sections)
        size_t shstrtab_size = 1; // Section NULL
        for (int i = 0; i < fusion_count; i++) {
            shstrtab_size += strlen(fusion[i].name) + 1; // nom + /0
        }
        shstrtab_size += strlen(".shstrtab") + 1; // son propre nom
        shstrtab_size += strlen(".symtab") + 1; // nom de la table des symboles
        shstrtab_size += strlen(".strtab") + 1; // nom de la table des chaînes
        char *shstrtab = malloc(shstrtab_size);
        if (!shstrtab) { perror("malloc shstrtab"); exit(1); }

        size_t offset = 1;
        shstrtab[0] = '\0';

        uint32_t offset_nom[fusion_count];
        for (int i = 0; i < fusion_count; i++) {
            offset_nom[i] = offset;
            strcpy(shstrtab + offset, fusion[i].name);
            offset += strlen(fusion[i].name) + 1;
        }
        uint32_t shstrtab_name_offset = offset;
        strcpy(shstrtab + offset, ".shstrtab");
        offset += strlen(".shstrtab") + 1;
        strcpy(shstrtab + offset, ".symtab");
        offset += strlen(".symtab") + 1;
        strcpy(shstrtab + offset, ".strtab");

        // Reconstruire une table complète : NULL + PROGBITS  + .symtab + .strtab + .shstrtab
        int total_sections = fusion_count + 4;
        Elf32_Shdr *full_sections = calloc(total_sections, sizeof(Elf32_Shdr));
        if (!full_sections) { perror("calloc full_sections"); exit(1); }

        // Section 0 : NULL
        memset(&full_sections[0], 0, sizeof(Elf32_Shdr));

        // Sections 1 à fusion_count
        for (int i = 0; i < fusion_count; i++) {
            int old_idx = fusion[i].idx_out;
            full_sections[i + 1] = out_ctx.sections[old_idx];
            full_sections[i + 1].sh_name = offset_nom[i];
            full_sections[i + 1].sh_addr = 0;
        }

        // Fusionner les tables de symboles
        charger_table_symboles(&ctx);
        charger_table_symboles(&ctx2);
        elf_table_sym_t* table_sym_fusion = fusion_tables_symboles(&ctx, &ctx2);
        if (table_sym_fusion == NULL) { perror("fusion_tables_symboles"); exit(1); }

        // Section .symtab
        int symtab_idx = total_sections - 3;
        full_sections[symtab_idx].sh_name = shstrtab_name_offset + strlen(".shstrtab") + 1; // Nom dans .shstrtab
        full_sections[symtab_idx].sh_type = SHT_SYMTAB;
        full_sections[symtab_idx].sh_flags = 0;
        full_sections[symtab_idx].sh_addr = 0;
        full_sections[symtab_idx].sh_offset = full_sections[symtab_idx - 1].sh_offset + full_sections[symtab_idx - 1].sh_size;
        full_sections[symtab_idx].sh_size = table_sym_fusion->nb_symboles * sizeof(Elf32_Sym);
        full_sections[symtab_idx].sh_addralign = 1;
        full_sections[symtab_idx].sh_entsize = sizeof(Elf32_Sym);
        full_sections[symtab_idx].sh_link = symtab_idx + 1; // Lien vers .strtab

        // Section .strtab
        int strtab_idx = total_sections - 2;
        //size_t strtab_size = table_sym_fusion->size_strtab; A CALCULER CORRECTEMENT

        full_sections[strtab_idx].sh_name = shstrtab_name_offset + strlen(".shstrtab") + 1 + strlen(".symtab") + 1; // Nom dans .shstrtab
        full_sections[strtab_idx].sh_type = SHT_STRTAB;
        full_sections[strtab_idx].sh_flags = 0;
        full_sections[strtab_idx].sh_addr = 0;
        full_sections[strtab_idx].sh_offset = full_sections[symtab_idx].sh_offset + full_sections[symtab_idx].sh_size;
        full_sections[strtab_idx].sh_size = 1000; // A CALCULER CORRECTEMENT
        full_sections[strtab_idx].sh_addralign = 1;
        full_sections[strtab_idx].sh_entsize = 0;
        full_sections[strtab_idx].sh_link = 0;

        // Section .shstrtab
        int shstrtab_idx = total_sections - 1;
        full_sections[shstrtab_idx].sh_name = shstrtab_name_offset;
        full_sections[shstrtab_idx].sh_type = SHT_STRTAB;
        full_sections[shstrtab_idx].sh_flags = 0;
        full_sections[shstrtab_idx].sh_addr = 0;
        full_sections[shstrtab_idx].sh_offset = 0; // À remplir plus tard
        full_sections[shstrtab_idx].sh_size = shstrtab_size;
        full_sections[shstrtab_idx].sh_addralign = 1;

        // Écrire les données des PROGBITS
        if (write_sections(&ctx, &ctx2, &out_ctx, fusion, fusion_count) != 0) {
            fprintf(stderr, "Erreur lors de l'écriture des sections\n");
            fclose(out_ctx.file);               
            free(shstrtab);                     
            if (out_ctx.sections) free(out_ctx.sections);  
            for (int i = 0; i < fusion_count; i++) free(fusion[i].name);
            free(fusion);
            fclose(f2);
            exit(1);
            exit(1);
        }

        // Écrire .symtab et .strtab
        inverse_Sym(table_sym_fusion->symboles, table_sym_fusion->nb_symboles);
        fseek(out_ctx.file, full_sections[symtab_idx].sh_offset, SEEK_SET);
        fwrite(table_sym_fusion->symboles, sizeof(Elf32_Sym), table_sym_fusion->nb_symboles, out_ctx.file);
        fwrite(table_sym_fusion->noms_symboles, 1, 1000, out_ctx.file); // A CALCULER LA TAILLE CORRECTEMENT

        detruire_table_sym(table_sym_fusion);


        // Écrire .shstrtab
        long shstrtab_offset = ftell(out_ctx.file);
        fwrite(shstrtab, 1, shstrtab_size, out_ctx.file);

        // Mettre à jour l'offset dans full_sections
        full_sections[shstrtab_idx].sh_offset = shstrtab_offset;

        // Écrire l'en-tête ELF
        Elf32_Ehdr ehdr_out;
        memset(&ehdr_out, 0, sizeof(ehdr_out));
        memcpy(ehdr_out.e_ident, ctx.ehdr.e_ident, EI_NIDENT);
        ehdr_out.e_type = ET_REL;
        ehdr_out.e_machine = EM_ARM;
        ehdr_out.e_version = EV_CURRENT;
        ehdr_out.e_entry = 0;
        ehdr_out.e_phoff = 0;
        ehdr_out.e_flags = ctx.ehdr.e_flags;
        ehdr_out.e_shnum = total_sections;
        ehdr_out.e_shentsize = sizeof(Elf32_Shdr);
        ehdr_out.e_shstrndx = shstrtab_idx;

        // Table des sections à la fin
        long table_offset = ftell(out_ctx.file);
        ehdr_out.e_shoff = table_offset;

        // Écrire la table des sections (big endian)
        for (int i = 0; i < total_sections; i++) {
            Elf32_Shdr sh_be = full_sections[i];
            inverse_Section(&sh_be, 1);
            fwrite(&sh_be, sizeof(Elf32_Shdr), 1, out_ctx.file);
        }

        // Réécrire l'en-tête ELF au début du fichier
        fseek(out_ctx.file, 0, SEEK_SET);
        Elf32_Ehdr ehdr_be = ehdr_out;
        inverse_entete(&ehdr_be);
        fwrite(&ehdr_be, sizeof(Elf32_Ehdr), 1, out_ctx.file);

        // Nettoyage
        free(shstrtab);
        free(out_ctx.sections);
        free(full_sections);
        for (int i = 0; i < fusion_count; i++) free(fusion[i].name);
        free(fusion);
        fclose(out_ctx.file);
        fclose(f2);

        printf("Fichier fusion.o généré avec succès. (%d sections PROGBITS)\n", fusion_count);
        printf("En-tête ELF, .shstrtab et table des sections inclus\n");
        printf("Tables des symboles fusionnées incluses\n");

    } else if (strcmp(commande, "-F") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: %s <file1.o> -F <file2.o>\n", argv[0]);
            exit(1);
        }
        
        // Ouvrir le deuxième fichier
        FILE* f2 = fopen(argv[3], "rb");
        if(!f2){
            if (errno == ENOENT){
                fprintf(stderr, "Erreur : le fichier '%s' n'existe pas.\n", argv[3]);
            } else {
                fprintf(stderr, "Erreur lors de l'ouverture du fichier.\n");
            }
            fclose(f);
            exit(1);
        }
        
        // Vérifier le deuxième fichier
        elf_contexte_t ctx2;
        memset(&ctx2, 0, sizeof(ctx2));
        
        if (!verifier_fichier(f2, &ctx2)){
            fprintf(stderr, "Le deuxième fichier n'est pas un ELF 32 bits ARM big endian.\n");
            fclose(f2);
            fclose(f);
            exit(1);
        }
        
        // Charger les sections et noms pour les deux fichiers
        lire_table_sections(&ctx);
        lire_noms_sections(&ctx);
        lire_table_sections(&ctx2);
        lire_noms_sections(&ctx2);
        
        // Charger les tables de réimplantation des deux fichiers
        charger_table_symboles(&ctx);
        charger_table_symboles(&ctx2);
        
        elf_tables_reimplantations* tables1 = charger_tables_reimplantations(&ctx);
        elf_tables_reimplantations* tables2 = charger_tables_reimplantations(&ctx2);
        
        if (!tables1 || !tables2) {
            fprintf(stderr, "Erreur lors du chargement des tables de réimplantation\n");
            if (tables1) liberer_tables_reimplantations(tables1);
            if (tables2) liberer_tables_reimplantations(tables2);
            fclose(f2);
            fclose(f);
            exit(1);
        }
        
        // Fusionner les tables des sections
        section_fusion_t *fusion = NULL;
        int fusion_count = 0;
        
        if (fusion_section_table(&ctx, &ctx2, &fusion, &fusion_count) != 0) {
            fprintf(stderr, "Erreur lors de la fusion des tables des sections\n");
            liberer_tables_reimplantations(tables1);
            liberer_tables_reimplantations(tables2);
            fclose(f2);
            fclose(f);
            exit(1);
        }
        
        // Calculer les offsets de concaténation
        concat_offset(&ctx, &ctx2, fusion, fusion_count);
        
        
        // Charger et fusionner les tables de symboles
        //lire_table_symboles(&ctx);
        //lire_table_symboles(&ctx2);

        elf_table_sym_t* table_sym_fusion = fusion_tables_symboles(&ctx, &ctx2);
        
        if (!table_sym_fusion) {
            fprintf(stderr, "Erreur lors de la fusion des tables de symboles\n");
            liberer_tables_reimplantations(tables1);
            liberer_tables_reimplantations(tables2);
            for (int i = 0; i < fusion_count; i++) free(fusion[i].name);
            free(fusion);
            fclose(f2);
            fclose(f);
            exit(1);
        }
        
        // Fusionner les tables de réimplantation
        elf_tables_reimplantations* tables_fusion = fusionner_tables_reimplantations(
            &ctx, &ctx2, tables1, tables2, fusion, fusion_count, table_sym_fusion);
        
        if (!tables_fusion) {
            fprintf(stderr, "Erreur lors de la fusion des tables de réimplantation\n");
        } else {
            // Afficher les tables fusionnées
            printf("=== TABLES DE REIMPLANTATION FUSIONNEES ===\n\n");
            afficher_tables_reimplantations(tables_fusion);
            liberer_tables_reimplantations(tables_fusion);
        }
        
        // Nettoyage
        liberer_tables_reimplantations(tables1);
        liberer_tables_reimplantations(tables2);
        detruire_table_sym(table_sym_fusion);
        for (int i = 0; i < fusion_count; i++) free(fusion[i].name);
        free(fusion);
        fclose(f2);
	} else {
	    fprintf(stderr, "Commande non reconnue: %s\n", commande);
	    usage(argv[0]);
	    fclose(f);
	    exit(1);
	}
	liberer_contexte(&ctx);
	fclose(f);
	exit(0);
}
