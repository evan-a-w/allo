CC = clang
CFLAGS = -Wall -Wextra -Werror -Wpedantic -g -fsanitize=address
NAME = allo

all: $(NAME)

$(NAME): $(NAME).c $(NAME).h
	$(CC) $(CFLAGS) $(NAME).c -o $(NAME)

