CC = clang
CFLAGS = -Wall -Wextra -Wpedantic -g -fsanitize=address

all: allo.a

allo.a: allo.o stats.o rb_tree.o allo.h
	ar rcs allo.a allo.o stats.o rb_tree.o

allo.o: allo.c allo.h
	$(CC) $(CFLAGS) allo.c -c -o allo.o

stats.o: stats.c stats.h
	$(CC) $(CFLAGS) stats.c -c -o stats.o

rb_tree.o: rb_tree.c rb_tree.h
	$(CC) $(CFLAGS) rb_tree.c -c -o rb_tree.o

clean:
	rm -f *.exe *.o *.a; make -C tests clean
