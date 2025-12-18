#ifndef __FUN_INV__
#define __FUN_INV__

/*permet d'inverser pour passer de big endian en little endian*/

Elf32_Ehdr* inverse_entete(Elf32_Ehdr *ehdr); 
Elf32_Shdr* inverse_ONE_Section(Elf32_Shdr *sh); 
Elf32_Shdr* inverse_Section(Elf32_Shdr *Tabsh, int n);
Elf32_Sym* inverse_ONE_Sym(Elf32_Sym *s);
Elf32_Sym* inverse_Sym(Elf32_Sym *Tabs, int n);
Elf32_Rel* inverse_REL(Elf32_Rel* rel);
Elf32_Rela* inverse_RELA(Elf32_Rela* rela);

#endif
