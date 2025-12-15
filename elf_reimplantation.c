#include <elf_symboles.h>

//////////////////////////////////////////
// A MODIFIER ATTENTION //
/*
- TROUVER LE DEBUT DE LA TABLE
*/
///////////////////////////////////

//Affiche les relations d'une entrée donée de type REL
void afficher_relations_entree_REL(Elf32_Rel* entree_rel)
{
    //Les printfs sont peut-être pas encore corrects, faut cast en string peut-être ?
    printf("%lu ", (unsigned long)entree_rel->r_offset);
    printf("%lu ", (unsigned long)entree_rel->r_info);

    //On extrait le type, et on l'associe à son équivalent texte
    unsigned char type_c = ELF32_R_TYPE(entree_rel->r_info);
    char type_s[16];
    switch (type_c)
    {
        case 0:
            strcpy(type_s, "STT_NOTYPE");
        case 1:
            strcpy(type_s, "STT_OBJECT");
        case 2:
            strcpy(type_s, "STT_FUNC");
        case 3:
            strcpy(type_s, "STT_SECTION");
        case 4:
            strcpy(type_s, "STT_FILE");
        case 13:
            strcpy(type_s, "STT_LOPROC");
        case 15:
            strcpy(type_s, "STT_HIPROC");
    }
    printf("%s ", type_s);

    //Sym. value, sym. name : utiliser table des symboles
    //On extrait l'index de table des symboles, et on l'associe à la table des symboles
    unsigned char sym_c = (unsigned char)ELF32_R_SYM(entree_rel->r_info);
}



//Affiche les relations d'une entrée donée de type RELA
void afficher_relations_entree_RELA(Elf32_Rela* entree_rela)
{
    printf("%lu ", (unsigned long)entree_rela->r_offset);
    printf("%lu ", (unsigned long)entree_rela->r_info);

    //On extrait le type, et on l'associe à son équivalent texte
    unsigned char type_c = ELF32_R_TYPE(entree_rela->r_info);
    char type_s[16];
    switch (type_c)
    {
        case 0:
            strcpy(type_s, "STT_NOTYPE");
        case 1:
            strcpy(type_s, "STT_OBJECT");
        case 2:
            strcpy(type_s, "STT_FUNC");
        case 3:
            strcpy(type_s, "STT_SECTION");
        case 4:
            strcpy(type_s, "STT_FILE");
        case 13:
            strcpy(type_s, "STT_LOPROC");
        case 15:
            strcpy(type_s, "STT_HIPROC");
    }
    printf("%s ", type_s);

    //Sym. value, sym. name : utiliser table des symboles
    //On extrait l'index de table des symboles, et on l'associe à la table des symboles
    unsigned char sym_c = (unsigned char)ELF32_R_SYM(entree_rela->r_info);


    printf("%ld\n", (signed long)entree_rela->r_addend);
}

//Itere sur une table de reimplantations, et affiche toutes les entrees
void afficher_table_relations(Elf32_Shdr* reitab)
{
    Elf32_Word type = reitab->sh_type;
    nb_relations = reitab->sh_size / reitab->sh_entsize;
    printf("Offset Info Type Sym.Value Sym.Name + Addend\n");
    if (type == SHT_RELA)
    {
        for (int i = 0; i < nb_relations; i++)
        {
            afficher_relations_entree_RELA(reitab->sh_link + i);
        }
    }
    else
    {
        for (int i = 0; i < nb_relations; i++)
        {
            afficher_relations_entree_REL(reitab->sh_link + i);
        }
    }
}