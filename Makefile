CC=gcc
CFLAGS=-lcrypto
NAME=pwmgr

pwmgr: src/pwmgr.c
<<<<<<< HEAD
<<<<<<< HEAD
	$(CC) -o $(NAME) src/pwmgr.c $(CFLAGS)
=======
	$(CC) -o $(NAME) src/main.c $(CFLAGS)
>>>>>>> 30651d0... renamed main.c to pwmgr.c
=======
	$(CC) -o $(NAME) src/pwmgr.c $(CFLAGS)
>>>>>>> 7ca14f2... renamed main.c to pwmgr.c

install: $(NAME)
	cp $(NAME) /usr/bin/$(NAME)

uninstall: /usr/bin/$(NAME)
	rm -f /usr/bin/$(NAME)

clean: $(NAME)
	rm $(NAME)
