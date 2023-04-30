CC = clang
CFLAGS = -Wall -Wextra -Wpedantic -g -fsanitize=address

main.exe: main.o allo.o stats.o rb_tree.o
	$(CC) $(CFLAGS) allo.o stats.o rb_tree.o main.o -o main.exe

main.o: main.c allo.h stats.h rb_tree.h
	$(CC) $(CFLAGS) main.c -c -o main.o

allo.o: allo.c allo.h stats.h rb_tree.h
	$(CC) $(CFLAGS) allo.c -c -o allo.o

stats.o: stats.c stats.h
	$(CC) $(CFLAGS) stats.c -c -o stats.o

rb_tree.o: rb_tree.c rb_tree.h
	$(CC) $(CFLAGS) rb_tree.c -c -o rb_tree.o

clean:
	rm -f *.exe *.o
