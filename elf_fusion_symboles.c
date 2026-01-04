#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <elf.h>
#include "elf_fusion_symboles.h"
#include "elf_symboles.h"

/*
typedef struct {
Elf32_Word st_name;
Elf32_Addr st_value;
Elf32_Word st_size;
unsigned char st_info;
unsigned char st_other;
Elf32_Half st_shndx;
} Elf32_Sym;
*/

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
elf_table_sep_t* isoler_globaux_table_sym(elf_table_sym_t* table)
{
	//Un premier tour pour calculer le nbre de GLOBAUX
	int nb_symboles_globaux = 0;
	for (int i = 0; i < table->nb_symboles; i++)
	{
		if (!strcmp(portee_symbole(table->symboles[i].st_info), "GLOBAL")) //Reutilisation d'une fonction de l'affichage des tables sym
		{
			nb_symboles_globaux += 1;
		}
	}
	
	//Comme ça, on a direct une taille optimisée
	elf_table_sym_t* table_globaux = construire_table_sym(nb_symboles_globaux);
	elf_table_sym_t* table_locaux = construire_table_sym(table->nb_symboles - nb_symboles_globaux);
	table_globaux->noms_symboles = table->noms_symboles;
	table_locaux->noms_symboles = table->noms_symboles;
	for (int i = 0; i < table->nb_symboles; i++)
	{
		if (!strcmp(portee_symbole(table->symboles[i].st_info), "GLOBAL")) //Reutilisation d'une fonction de l'affichage des tables sym
		{
			table_globaux->symboles[i] = table->symboles[i];
		} else
		{
			table_locaux->symboles[i] = table->symboles[i];
		}
	}
	
	elf_table_sep_t* table_separee = malloc(sizeof(elf_table_sep_t));
	table_separee->locaux = table_locaux;
	table_separee->globaux = table_globaux;
	return table_separee;
}

//Fusionne les tables table1 et table2, puis les détruit
//TODO: NE PAS OUBLIER D'AJOUTER L'OFFSET AUX VALEURS DE TABLE 2 -> Probablement à faire lors des fusions ?
elf_table_sym_t* construire_table_sym_fusion(elf_table_sym_t* table1, elf_table_sym_t* table2)
{	
	//On isole d'abord les LOCAUX et les GLOBAUX
	elf_table_sep_t* table_separee_1 = isoler_globaux_table_sym(table1);
	elf_table_sep_t* table_separee_2 = isoler_globaux_table_sym(table2);
	
	//La taille comprendra déjà tous les locaux des deux
	int nb_symboles_uniques = (table_separee_1->locaux->nb_symboles + table_separee_2->locaux->nb_symboles);
	
	
	
	//On fait une table de la fusion des GLOBAUX, pour simplifier 
	int nb_symboles_globaux_max = (table_separee_1->globaux->nb_symboles + table_separee_2->globaux->nb_symboles);
	elf_table_sym_t* table_global_temp = construire_table_sym(nb_symboles_globaux_max);
	
	//On ajoute tous ceux GLOBAUX de table 1
	for (int i = 0; i < table_separee_1->globaux->nb_symboles; i++)
	{
		table_global_temp->symboles[i] = table_separee_1->globaux->symboles[i];
	}
	
	//Et on rajoute après les GLOBAUX de table 2
	int offset_sym_globaux = table_separee_1->globaux->nb_symboles;
	int skipped = 0; //Pour permettre de skip certains quand il faut insérer : placés autrement + non placés
	for (int i = offset_sym_globaux; i < offset_sym_globaux + table_separee_2->globaux->nb_symboles; i++)
	{
		//On regarde si le symbole est déjà présent
		Elf32_Sym current_sym = table_separee_2->globaux->symboles[i - offset_sym_globaux];
		for (int j = 0; j < offset_sym_globaux; j++)
		{
			//Comparaison du nom du symoble courrant avec le reste de la table
			Elf32_Sym comp_sym = table_global_temp->symboles[j];
			if (!strcmp(table2->noms_symboles + current_sym.st_name, table1->noms_symboles + comp_sym.st_name))
			{
				//On regarde si les deux sont définis, puis on agit en fonction des cas
				int def1 = (comp_sym.st_shndx != 0);
				int def2 = (comp_sym.st_shndx != 0);
				if (def1 && def2)
				{
					//Erreur : défini deux fois
					fprintf(stderr, "ERREUR : UN SYMBOLE GLOBAL A DEUX DEFINITIONS");
					return NULL;
				} else if (def2)
				{
					//Def dans t2 -> On remplace l'ancienne entree par la nouvelle
					//TODO: PRENDRE EN COMPTE L'OFFSET POUR SON NOM
					table_global_temp->symboles[j] = current_sym;
					skipped++;
				} else
				{
					//Déf dans t1 et pas dans t2 OU def dans aucune des deux -> On touche à rien
					skipped++;
				}
			} else {
				table_global_temp->symboles[i - skipped] = current_sym;
				nb_symboles_uniques++;
			}
		}
	}
	
	
	
	//On peut enfin tout mettre ensemble !
	//LOCAL 1, LOCAL 2, puis GLOBAL
	elf_table_sym_t* table_fusion = construire_table_sym(nb_symboles_uniques);
	int offset_fusion = table_separee_1->locaux->nb_symboles;
	for (int i = 0; i < offset_fusion; i++)
	{
		table_fusion->symboles[i] = table_separee_1->locaux->symboles[i];
	}
	for (int i = 0; i < table_separee_2->locaux->nb_symboles; i++)
	{
		table_fusion->symboles[offset_fusion + i] = table_separee_2->locaux->symboles[i];
	}
	offset_fusion += table_separee_2->locaux->nb_symboles;
	for (int i = 0; i < table_global_temp->nb_symboles; i++)
	{
		table_fusion->symboles[offset_fusion + i] = table_global_temp->symboles[i];
	}
	
	detruire_table_sym(table1);
	detruire_table_sym(table2);
  free(table_separee_1);
  free(table_separee_2);
	return table_fusion;
}