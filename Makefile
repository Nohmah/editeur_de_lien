CC=gcc
CFLAGS=-Wall -Werror

elf_reader: elf_reader.o elf_header.o elf_section_table.o elf_symboles.o
	$(CC) -o $@ $^
%.o: %.c
	$(CC) $(CFLAGS) -c $<
elf_reader.o: elf_header.h elf_section_table.h
elf_header.o: elf_header.h
elf_section_table.o: elf_section_table.h

clean:
	rm -f *.o elf_reader