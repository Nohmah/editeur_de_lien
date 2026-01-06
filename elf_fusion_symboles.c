#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <elf.h>
#include "elf_fusion_symboles.h"
#include "elf_symboles.h"


//Decommentez seulement si vous voulez faire des tests sur cette partie
//#include <errno.h>
//#include "elf_header.h"


//Fusionne des tables de string en 1 dimension
char* fusionner_strings_1_dim(char* str1, char* str2, int len1, int len2)
{
  char* str_fusion = malloc(len1 + len2);
  for (int i = 0; i < len1; i++)
  {
    str_fusion[i] = str1[i];
  }
  for (int i = 0; i < len2; i++)
  {
    str_fusion[i + len1] = str2[i];
  }
  return str_fusion;
}

elf_table_sym_t* construire_table_sym(int size)
{
	elf_table_sym_t* table = malloc(sizeof(elf_table_sym_t));
	if (size > 0) table->symboles = malloc(sizeof(Elf32_Sym) * size);
	else table->symboles = NULL;
	table->noms_symboles = NULL;
	table->nb_symboles = size;
	return table;
}

void detruire_table_sym(elf_table_sym_t* table)
{
	free(table->symboles);
	free(table->noms_symboles);
	free(table);
}

//OPTI : On peut aussi d'abord allouer nb_symobles, puis realloc à la fin
//J'ai du mettre le calcul de la taille des tables de string la dedans
//Tables de string à 1 dimension = AU SECOURS
elf_table_sep_t* isoler_globaux_table_sym(elf_table_sym_t* table)
{
	//Un premier tour pour calculer le nbre de GLOBAUX et calculer la taille de la TABLE de STRING
	int nb_symboles_globaux = 0;
  int taille_string = 0;
	for (int i = 0; i < table->nb_symboles; i++)
	{
		if (!strcmp(portee_symbole(table->symboles[i].st_info), "GLOBAL")) //Reutilisation d'une fonction de l'affichage des tables sym
		{
			nb_symboles_globaux += 1;
		}
    taille_string += (strlen(table->noms_symboles + table->symboles[i].st_name) + 1);
	}
	
	//Comme ça, on a direct une taille optimisée
	elf_table_sym_t* table_globaux = construire_table_sym(nb_symboles_globaux);
	elf_table_sym_t* table_locaux = construire_table_sym(table->nb_symboles - nb_symboles_globaux);
	table_globaux->noms_symboles = table->noms_symboles;
	table_locaux->noms_symboles = table->noms_symboles;
  int i_local = 0;
  int i_global = 0;
	for (int i = 0; i < table->nb_symboles; i++)
	{
		if (!strcmp(portee_symbole(table->symboles[i].st_info), "GLOBAL")) //Reutilisation d'une fonction de l'affichage des tables sym
		{
			table_globaux->symboles[i_global] = table->symboles[i];
      i_global++;
		} else
		{
			table_locaux->symboles[i_local] = table->symboles[i];
      i_local++;
		}
               
	}
	elf_table_sep_t* table_separee = malloc(sizeof(elf_table_sep_t));
	table_separee->locaux = table_locaux;
	table_separee->globaux = table_globaux;
  table_separee->taille_string = taille_string;
	return table_separee;
}

//Fusionne les tables table1 et table2, puis les détruit
//TODO: NE PAS OUBLIER D'AJOUTER L'OFFSET AUX VALEURS DE TABLE 2 -> Probablement à faire lors des fusions ?
elf_table_sym_t* construire_table_sym_fusion(elf_table_sym_t* table1, elf_table_sym_t* table2)
{	
	//On isole d'abord les LOCAUX et les GLOBAUX
	elf_table_sep_t* table_separee_1 = isoler_globaux_table_sym(table1);
	elf_table_sep_t* table_separee_2 = isoler_globaux_table_sym(table2);
  int offset_noms_sym = table_separee_1->taille_string;
	
	//La taille comprendra déjà tous les locaux des deux
	int nb_symboles_uniques = (table_separee_1->locaux->nb_symboles + table_separee_2->locaux->nb_symboles);
  int nb_symboles_globaux = 0;
	
	
	//On fait une table de la fusion des GLOBAUX, pour simplifier 
	int nb_symboles_globaux_max = (table_separee_1->globaux->nb_symboles + table_separee_2->globaux->nb_symboles);
	elf_table_sym_t* table_global_temp = construire_table_sym(nb_symboles_globaux_max);
	
	//On ajoute tous ceux GLOBAUX de table 1
	for (int i = 0; i < table_separee_1->globaux->nb_symboles; i++)
	{
		table_global_temp->symboles[i] = table_separee_1->globaux->symboles[i];
    nb_symboles_globaux++;
	}
	
	//Et on rajoute après les GLOBAUX de table 2
	int offset_sym_globaux = table_separee_1->globaux->nb_symboles;
	int skipped = 0; //Pour permettre de skip certains quand il faut insérer : placés autrement + non placés
	for (int i = 0; i < table_separee_2->globaux->nb_symboles; i++)
	{
		//On regarde si le symbole est déjà présent
		Elf32_Sym current_sym = table_separee_2->globaux->symboles[i];
    int trouve = 1;
		for (int j = 0; j < offset_sym_globaux; j++)
		{
			//Comparaison du nom du symoble courrant avec le reste de la table
			Elf32_Sym comp_sym = table_global_temp->symboles[j];
			if (!strcmp(table2->noms_symboles + current_sym.st_name, table1->noms_symboles + comp_sym.st_name))
			{
        trouve = 0;
				//On regarde si les deux sont définis, puis on agit en fonction des cas
				int def1 = strcmp(type_symbole(comp_sym.st_info), "NOTYPE");
				int def2 = strcmp(type_symbole(current_sym.st_info), "NOTYPE");
				if (def1 && def2)
				{
					//Erreur : défini deux fois
					fprintf(stderr, "ERREUR : UN SYMBOLE GLOBAL A DEUX DEFINITIONS");
					return NULL;
				} else if (def2)
				{
					//Def dans t2 -> On remplace l'ancienne entree par la nouvelle
					//TODO: PRENDRE EN COMPTE L'OFFSET POUR SON NOM
          current_sym.st_name = comp_sym.st_name;
					table_global_temp->symboles[j] = current_sym;
					skipped++;
				} else
				{
					//Déf dans t1 et pas dans t2 OU def dans aucune des deux -> On touche à rien
					skipped++;
				}
			}
    }
    if (trouve) {
      current_sym.st_name += offset_noms_sym;
      table_global_temp->symboles[i + offset_sym_globaux - skipped] = current_sym;
      nb_symboles_globaux++;
    }
  }
  
  nb_symboles_uniques += nb_symboles_globaux;
  table_global_temp->nb_symboles = nb_symboles_globaux;
  
  
	
	//On peut enfin tout mettre ensemble !
	//LOCAL 1, LOCAL 2, puis GLOBAL
	elf_table_sym_t* table_fusion = construire_table_sym(nb_symboles_uniques);
  table_fusion->noms_symboles = fusionner_strings_1_dim(
  table1->noms_symboles, table2->noms_symboles,
  table_separee_1->taille_string, table_separee_2->taille_string);
  
	int offset_fusion = table_separee_1->locaux->nb_symboles;
  Elf32_Sym curr_sym;
	for (int i = 0; i < offset_fusion; i++)
	{
    curr_sym = table_separee_1->locaux->symboles[i];
		table_fusion->symboles[i] = curr_sym;
	}
	for (int i = 0; i < table_separee_2->locaux->nb_symboles; i++)
	{
    curr_sym = table_separee_2->locaux->symboles[i];
    curr_sym.st_name += offset_noms_sym;
		table_fusion->symboles[offset_fusion + i] = curr_sym;
	}
	offset_fusion += table_separee_2->locaux->nb_symboles;
  for (int i = 0; i < table_global_temp->nb_symboles; i++)
	{
    curr_sym = table_global_temp->symboles[i];
		table_fusion->symboles[offset_fusion + i] = curr_sym;
	}
	
  // Free the temporary separated tables but NOT the original data
  free(table_separee_1->locaux->symboles);
  free(table_separee_1->locaux);
  free(table_separee_1->globaux->symboles);
  free(table_separee_1->globaux);
  free(table_separee_1);
  
  free(table_separee_2->locaux->symboles);
  free(table_separee_2->locaux);
  free(table_separee_2->globaux->symboles);
  free(table_separee_2->globaux);
  free(table_separee_2);
  
  free(table_global_temp->symboles);
  free(table_global_temp);
  
  // CRITICAL FIX: Don't destroy table1 and table2 - just free the wrapper structures
  free(table1);
  free(table2);
  
	return table_fusion;
}

// FIXED VERSION: Make deep copies instead of shallow copies
elf_table_sym_t* ctx_vers_fusion(elf_contexte_t* ctx)
{
  elf_table_sym_t* table = malloc(sizeof(elf_table_sym_t));
  
  // Deep copy symboles
  table->nb_symboles = ctx->nb_symboles;
  table->symboles = malloc(ctx->nb_symboles * sizeof(Elf32_Sym));
  memcpy(table->symboles, ctx->symboles, ctx->nb_symboles * sizeof(Elf32_Sym));
  
  // Deep copy noms_symboles - find the size first
  int len = 1;  // At least 1 for the initial null byte
  for (int i = 0; i < ctx->nb_symboles; i++) {
    if (ctx->symboles[i].st_name > 0) {
      int end = ctx->symboles[i].st_name + strlen(ctx->noms_symboles + ctx->symboles[i].st_name) + 1;
      if (end > len) len = end;
    }
  }
  
  table->noms_symboles = malloc(len);
  memcpy(table->noms_symboles, ctx->noms_symboles, len);
  
  return table;
}


//La fonction qui sera utilisée pour la fin, transforme 2 contexte en un objet table de symboles
elf_table_sym_t* fusion_tables_symboles(elf_contexte_t* ctx1, elf_contexte_t* ctx2)
{
  elf_table_sym_t* table1 = ctx_vers_fusion(ctx1);
  elf_table_sym_t* table2 = ctx_vers_fusion(ctx2);
  return construire_table_sym_fusion(table1,table2);
}


// LE MAIN SERT A FAIRE DES TESTS SUR CETTE PARTIE EN PARTICULIER.

/*int main()
{
  FILE* f1 = fopen("file1.o", "rb");
	if(!f1){
		if (errno == ENOENT){
	        fprintf(stderr, "Erreur : le fichier file1.o n'existe pas.\n");
		} else {
			fprintf(stderr, "Erreur lors de l'ouverture du fichier.\n");
		}
		exit(1);
	}
	if (!verifier_fichier(f1)){
	    fprintf(stderr, "Ce programme ne supporte que les fichiers ELF 32 bits ARM big endian.\n");
	    exit(1);
	}
  
  FILE* f2 = fopen("file2.o", "rb");
	if(!f2){
		if (errno == ENOENT){
	        fprintf(stderr, "Erreur : le fichier file2.o n'existe pas.\n");
		} else {
			fprintf(stderr, "Erreur lors de l'ouverture du fichier.\n");
		}
		exit(2);
	}
	if (!verifier_fichier(f2)){
	    fprintf(stderr, "Ce programme ne supporte que les fichiers ELF 32 bits ARM big endian.\n");
	    exit(1);
	}
  
  elf_contexte_t ctx1;
	    memset(&ctx1, 0, sizeof(ctx1));
	    ctx1.file = f1;
	    lire_entete_elf(&ctx1);
	    lire_table_sections(&ctx1);
	    lire_noms_sections(&ctx1);
	    lire_table_symboles(&ctx1);
      
  elf_contexte_t ctx2;
	    memset(&ctx2, 0, sizeof(ctx2));
	    ctx2.file = f2;
	    lire_entete_elf(&ctx2);
	    lire_table_sections(&ctx2);
	    lire_noms_sections(&ctx2);
	    lire_table_symboles(&ctx2);
      
  elf_table_sym_t* table = fusion_tables_symboles(&ctx1, &ctx2);
  
  char* nom;
  printf("TABLE FUSIONEE\n");
  printf("Index  Valeur    Type     Portée   Section  Nom\n");
  for (int i = 0; i < table->nb_symboles; i++) {
    nom = table->noms_symboles + table->symboles[i].st_name;
    printf("%5d  %08x %-8s %-8s %7d  %s\n",
               i,
               table->symboles[i].st_value,
               type_symbole(table->symboles[i].st_info),
               portee_symbole(table->symboles[i].st_info),
               table->symboles[i].st_shndx,
               nom);
  }
}*/