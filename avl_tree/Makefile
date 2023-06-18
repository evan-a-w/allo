CC = clang
CFLAGS = -Wall -Wextra -Wpedantic -g -fsanitize=address -I..

avl_tree.o: avl_tree.c avl_tree.h ../allo.h
	$(CC) $(CFLAGS) -c avl_tree.c

avl_tree_test.exe: avl_tree.o avl_tree_test.c avl_tree.h ../allo.a
	$(CC) $(CFLAGS) avl_tree_test.c avl_tree.o ../allo.a -o avl_tree_test.exe

test: avl_tree_test.exe
	unbuffer ./avl_tree_test.exe

../allo.a:
	$(MAKE) -C..

.PHONY: ../allo.a

clean:
	rm -f *.exe *.o *.a
