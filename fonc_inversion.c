#include <elf.h>
#include "util.h"
#include "fonc_inversion.h"

// inverse une en-tête 
Elf32_Ehdr* inverse_entete(Elf32_Ehdr *ehdr){
    ehdr->e_type = reverse_2(ehdr->e_type);
    ehdr->e_machine = reverse_2(ehdr->e_machine);
    ehdr->e_version = reverse_4(ehdr->e_version);
    ehdr->e_entry = reverse_4(ehdr->e_entry);
    ehdr->e_phoff = reverse_4(ehdr->e_phoff);
    ehdr->e_shoff = reverse_4(ehdr->e_shoff);
    ehdr->e_flags = reverse_4(ehdr->e_flags);
    ehdr->e_ehsize = reverse_2(ehdr->e_ehsize);
    ehdr->e_phentsize = reverse_2(ehdr->e_phentsize);
    ehdr->e_phnum = reverse_2(ehdr->e_phnum);
    ehdr->e_shentsize = reverse_2(ehdr->e_shentsize);
    ehdr->e_shnum = reverse_2(ehdr->e_shnum);
    ehdr->e_shstrndx = reverse_2(ehdr->e_shstrndx);
    return ehdr;
}

Elf32_Shdr* inverse_ONE_Section(Elf32_Shdr *sh){
    sh->sh_name = reverse_4(sh->sh_name);
    sh->sh_type = reverse_4(sh->sh_type);
    sh->sh_addr = reverse_4(sh->sh_addr);
    sh->sh_offset = reverse_4(sh->sh_offset);
    sh->sh_size = reverse_4(sh->sh_size);
    sh->sh_entsize = reverse_4(sh->sh_entsize);
    sh->sh_flags = reverse_4(sh->sh_flags);
    sh->sh_link = reverse_4(sh->sh_link);
    sh->sh_info = reverse_4(sh->sh_info);
    sh->sh_addralign = reverse_4(sh->sh_addralign);
    return sh;
}

Elf32_Shdr* inverse_Section(Elf32_Shdr *Tabsh, int n){
    for(int i = 0; i < n ; i++){
        inverse_ONE_Section(Tabsh + i);
    }
    return Tabsh;
}

Elf32_Sym* inverse_ONE_Sym(Elf32_Sym *s){
    s->st_name  = reverse_4(s->st_name);
    s->st_value = reverse_4(s->st_value);
    s->st_size  = reverse_4(s->st_size);
    s->st_shndx = reverse_2(s->st_shndx);
    return s;
}

Elf32_Sym* inverse_Sym(Elf32_Sym *Tabs, int n){
    for(int i = 0; i < n ; i++){
        inverse_ONE_Sym(Tabs + i);
    }
    return Tabs;
}


Elf32_Rel* inverse_REL(Elf32_Rel* rel)
{
    rel->r_offset = reverse_4(rel->r_offset);
    rel->r_info = reverse_4(rel->r_info);
    return rel;
}

Elf32_Rela* inverse_RELA(Elf32_Rela* rela)
{
    rela->r_offset = reverse_4(rela->r_offset);
    rela->r_info = reverse_4(rela->r_info);
    rela->r_addend = reverse_4(rela->r_addend);
    return rela;
}