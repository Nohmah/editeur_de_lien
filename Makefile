CC=gcc
CFLAGS=-Wall -Werror

elf_reader: elf_reader.o elf_header_and_section.o elf_symboles.o
	$(CC) -o $@ $^
%.o: %.c
	$(CC) $(CFLAGS) -c $<
elf_reader.o: elf_header_and_section.h
elf_header_and_section.o: elf_header_and_section.h

clean:
	rm -f *.o elf_reader