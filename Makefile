CC=gcc
CFLAGS=-Wall -Werror

elf_reader: elf_reader.o elf_header.o elf_section_table.o elf_lecture_section.o elf_symboles.o elf_reimplantation.o fonc_inversion.o fusion_table.o elf_fusion_symboles.o
	$(CC) -o $@ $^
%.o: %.c
	$(CC) $(CFLAGS) -c $<
elf_reader.o: elf_reader.c elf_header.h elf_section_table.h elf_symboles.h
elf_section_table.o: elf_section_table.c elf_section_table.h
elf_symboles.o: elf_symboles.c elf_symboles.h fonc_inversion.h
elf_lecture_section.o: elf_lecture_section.c elf_lecture_section.h
elf_reimplantation.o: elf_reimplantation.c elf_reimplantation.h elf_symboles.h
fonc_inversion.o: fonc_inversion.c fonc_inversion.h
fusion_table.o: fusion_table.c fusion_table.h
elf_fusion_symboles.o: elf_fusion_symboles.c elf_fusion_symboles.h elf_symboles.h

clean:
	rm -f *.o elf_reader

test: elf_reader
	@ERREUR=0; for script in tests/*.sh; do "$$script" || ERREUR=1; done; if [ $$ERREUR -eq 1 ]; then echo "IL Y A AU MOINS UN TEST FAUX"; else echo "TOUS LES TESTS SONT JUSTES"; fi