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

void usage(char *programme) {
    fprintf(stderr, "\nUsage:\n");
    fprintf(stderr, "%s <fichier ELF> <commande>\n\n", programme);
    fprintf(stderr, "Commandes disponibles :\n");
    fprintf(stderr, "  -h                 Afficher l'en-tête\n");
    fprintf(stderr, "  -S                 Afficher la table des sections\n");
    fprintf(stderr, "  -x <section>       Afficher le contenu d'une section\n");
    fprintf(stderr, "  -s                 Afficher la table des symboles\n");
    fprintf(stderr, "  -r                 Afficher les tables de réimplantation\n");
    fprintf(stderr, "  --help             Afficher ce message\n");
    fprintf(stderr, "Pour -x, spécifiez le nom ou le numéro de la section\n");
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
        concat_offset(&ctx, &ctx2, fusion, fusion_count);

        out.file = fopen("fusion.o", "wb+");
        if (!out.file) { perror("fusion.o"); exit(1); }

        build_output_section_headers(&ctx, &ctx2, &out, fusion, fusion_count);
        write_sections(&ctx, &ctx2, &out, fusion, fusion_count);

        printf("Fichier fusion.o généré avec %d sections PROGBITS.\n", fusion_count);

        for (int i = 0; i < fusion_count; i++) free(fusion[i].name);
        free(fusion);
        fclose(out.file);
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
