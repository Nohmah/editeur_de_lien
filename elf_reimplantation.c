
//Affiche les relations d'une entrée donée
void lire_relation_entree(elf_contexte_t *ctx)
{
    Elf32_Rela *entree_rela;

    //Les printfs sont peut-être pas encore corrects, faut cast en string peut-être ?
    printf("%s", entree_rela->r_offset);
    printf("%s", entree_rela->r_info);

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
    printf("%s", type_s);

    //Sym. value, sym. name : utiliser table des symboles
    //On extrait l'index de table des symboles, et on l'associe à la table des symboles
    unsigned char sym_c = (unsigned char)ELF32_R_SYM(entree_rela->r_info);

    printf("%d", entree_rela->r_addend);
}