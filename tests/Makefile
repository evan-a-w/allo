CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -g -fsanitize=address -I.. -lm

all: test_simple test_hash test_lzw

test_simple: simple.exe
	unbuffer ./simple.exe

test_hash: hash_table.exe
	unbuffer ./hash_table.exe

hash_table.exe: hash_table.c ../allo.a hash_table.h
	$(CC) $(CFLAGS) hash_table.c ../allo.a -o hash_table.exe

simple.exe: simple.c ../allo.a
	$(CC) $(CFLAGS) simple.c ../allo.a -o simple.exe

lencode.exe: lencode.c ../allo.a
	$(CC) $(CFLAGS) lencode.c ../allo.a -o lencode.exe

ldecode.exe: ldecode.c ../allo.a
	$(CC) $(CFLAGS) ldecode.c ../allo.a -o ldecode.exe

bible.txt:
	cp ~/crypt/bible.txt .

bible.encoded: lencode.exe bible.txt
	./lencode.exe bible.txt bible.encoded

bible.decoded: ldecode.exe bible.encoded
	./ldecode.exe bible.encoded bible.decoded

test_lzw: bible.decoded
	diff bible.txt bible.decoded

../allo.a:
	$(MAKE) -C.. CFLAGS="$(CFLAGS)"

.PHONY: ../allo.a

clean:
	rm -f *.exe *.o *.a *.encoded *.decoded
