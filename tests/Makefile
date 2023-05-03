CC = clang
CFLAGS = -Wall -Wextra -Wpedantic -g -fsanitize=address -I..

all: ../allo.a simple.exe hash_table.exe

test_hash: hash_table.exe
	unbuffer ./hash_table.exe

hash_table.exe: hash_table.c ../allo.a hash_table.h
	$(CC) $(CFLAGS) hash_table.c ../allo.a -o hash_table.exe

simple.exe: simple.c ../allo.a
	$(CC) $(CFLAGS) simple.c ../allo.a -o simple.exe

../allo.a:
	$(MAKE) -C..

.PHONY: ../allo.a

clean:
	rm -f *.exe *.o *.a
