CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -g -fsanitize=address -DDIE

all: allo.a

allo.a: allo.o stats.o avl_tree/avl_tree.o allo.h
	ar rcs allo.a allo.o stats.o avl_tree/avl_tree.o

allo.o: allo.c allo.h
	$(CC) $(CFLAGS) allo.c -c -o allo.o

stats.o: stats.c stats.h
	$(CC) $(CFLAGS) stats.c -c -o stats.o

avl_tree/avl_tree.o: avl_tree/avl_tree.c avl_tree/avl_tree.h
	make -Cavl_tree

clean:
	rm -f *.exe *.o *.a; make -C tests clean; make -C avl_tree clean
