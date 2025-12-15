#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "elf_header_and_section.h"
#include "elf_symboles.h"

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
		fprintf(stderr, "Erreur lors de l'ouverture du fichier.\n");
		exit(1);
	}
	if (!check_spec(f)) {
	    fprintf(stderr, "Ce programme ne supporte que les fichiers ELF 32 bits ARM big endian.\n");
	    exit(1);
	}

	// Exécution de la commande demandée
	char *commande;
	commande = argv[2];

	if (strcmp(commande, "-h") == 0) {
    	elf_header(f);
	}
	else if (strcmp(commande, "-S") == 0) {
	    elf_section(f);
	}
	else if (strcmp(commande, "-x") == 0) {
		printf("pas fait\n");
	}
	else if (strcmp(commande, "-s") == 0) {
	    elf_contexte_t ctx;
	    memset(&ctx, 0, sizeof(ctx));
	    ctx.fd = open(argv[1], O_RDONLY);
	    lire_entete_elf(&ctx);
	    lire_table_sections(&ctx);
	    lire_noms_sections(&ctx);
	    lire_table_symboles(&ctx);
	    liberer_contexte(&ctx);
	    close(ctx.fd);
	}
	else if (strcmp(commande, "-r") == 0) {
	    printf("pas fait\n");
	}
	else {
	    fprintf(stderr, "Commande non reconnue: %s\n", commande);
	    usage(argv[0]);
	    fclose(f);
	    exit(1);
	}
	fclose(f);
	exit(0);
}
