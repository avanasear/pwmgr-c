CC=gcc
CFLAGS=-lcrypto
NAME=pwmgr

pwmgr: src/pwmgr.c
	$(CC) -o $(NAME) src/pwmgr.c $(CFLAGS)

install: $(NAME)
	cp $(NAME) /usr/bin/$(NAME)

uninstall: /usr/bin/$(NAME)
	rm -f /usr/bin/$(NAME)

clean: $(NAME)
	rm $(NAME)
