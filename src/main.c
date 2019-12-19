#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>
//#include <openssl/>
//#include <openssl/>
#include <security/pam_appl.h>

struct termios saved_attributes;

void reset_input_mode();
void set_input_mode();

int init(); // not finished yet
int deinit(); // not finished yet
int is_initialized(); // not finished yet

int main(){
    char * user = getenv("USER");
    char * homedir = getenv("HOME");
    printf("You are %s.\n Your home directory is %s", user, homedir);

    return 0;
}

void reset_input_mode(){
    // Show characters on stdin again.
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}

void set_input_mode(){
    // Stop showing characters on stdin.
    struct termios tattr;
    if (!isatty (STDIN_FILENO)){
        fprintf(stderr, "Not a terminal.\n");
        exit(1);
    }
    tcgetattr(STDIN_FILENO, &saved_attributes);
    atexit(reset_input_mode);
    tcgetattr(STDIN_FILENO, &tattr);
    tattr.c_lflag &= ~(ICANON | ECHO);
    tattr.c_cc[VMIN] = 1;
    tattr.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
}

