CC = clang
CFLAGS = -Wall -Wextra -Wpedantic -g -fsanitize=address -I..

all: simple.exe

simple.exe: simple.o allo.a
	$(CC) $(CFLAGS) simple.o allo.a -o simple.exe

simple.o: simple.c ../allo.h
	$(CC) $(CFLAGS) simple.c -c -o simple.o

allo.a: ../allo.h
	$(MAKE) -C.. && cp ../allo.a .

clean:
	rm -f *.exe *.o *.a
