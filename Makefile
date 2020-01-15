CC=gcc
CFLAGS="-lcrypto"

NAME="pwmgr"
DEPS="src/pwmgr.h"

pwmgr: src/main.c
	$(CC) -o $(NAME) src/main.c $(DEPS) $(CFLAGS)

install: $(NAME)
	cp $(NAME) /usr/bin/$(NAME)

uninstall: /usr/bin/$(NAME)
	rm -f /usr/bin/$(NAME)

clean: $(NAME)
	rm $(NAME)
