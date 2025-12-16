CC=gcc
CFLAGS=-Wall -Werror

elf_reader: elf_reader.o elf_header.o elf_section_table.o elf_symboles.o
	$(CC) -o $@ $^
%.o: %.c
	$(CC) $(CFLAGS) -c $<
elf_reader.o: elf_reader.c elf_header.h elf_section_table.h elf_symboles.h
elf_section_table.o: elf_section_table.c elf_section_table.h
elf_symboles.o: elf_symboles.c elf_symboles.h

clean:
	rm -f *.o elf_reader