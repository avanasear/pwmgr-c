#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gcrypt.h>
//#include <json/json.h>
#include <termios.h>
#include <dirent.h>

char * HELP = "pwmgr-c: a WIP password manager, written in C\n\
\n\
 FIRST USAGE:\n\
 pwmgr [initialize || i]\n\
\n\
 AFTER FIRST USAGE:\n\
 pwmgr [add    || a]       <service>\n\
       [get    || g]       <service>\n\
       [list   || ls || l] <service>\n\
       [remove || rm || r] <service>\n\
       [sanitize]          <service>\n\
\n\
 INFO:\n\
 initialize - create ~/.pwmgr and the necessary files\n\
\n\
 add      - add an account\n\
 get      - get the password for an account\n\
 list     - show all stored passwords\n\
 remove   - remove an account\n\
 sanitize - remove all accounts and delete ~/.pwmgr\n\
            this is essentially uninstalling pwmgr.\n\
            you'll have to reinitialize pwmgr afterwards.\n\n";

struct termios saved_attributes;

void reset_input_mode(){
	// Show characters on terminal input again.
	tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}

void set_input_mode(){
	// Stop showing characters on terminal input.
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

int is_initialized(const char * dir, const char * shadow, const char * passes){
	/* Function to tell if:
	    a. The directory exists and
	    b. The files exist */

	if (access(dir, F_OK) != -1){
		if (access(shadow, F_OK) != -1){
			if (access(passes, F_OK) != -1){
				return 0;
			}
			else {
				return 1;
			}
		}
		else {
			return 1;
		}
	}
	else {
		return 1;
	}
}

void initialize(const char * dir, const char * shadow, const char * passes){
	// Create the necessary files and directory.

	struct stat st = { 0 };
	int initialized = 0;
	char passwd[100] = {0};
	char repeat[100] = {0};

	if (stat(dir, &st) == -1){
		mkdir(dir, 0700);
	}
	else {
		printf("~/.pwmgr already exists\n");
		initialized++;
	}

	if (stat(shadow, &st) == -1){
		FILE * shadowptr = fopen(shadow, "w");
		fputs("\n", shadowptr);
		fclose(shadowptr);
		chmod(shadow, 0600);
	}
	else {
		printf("~/.pwmgr/shadow.pw already exists\n");
		initialized++;
	}

	if (stat(passes, &st) == -1){
		FILE * passesptr = fopen(passes, "w");
		fputs("[]", passesptr);
		fclose(passesptr);
		chmod(passes, 0600);
	}
	else {
		printf("~/.pwmgr/passes.pw already exists\n");
		initialized++;
	}
	
	if (initialized < 1){
		// get passwd and repeat
		set_input_mode();

		printf("Create a password: ");
		fgets(passwd, 100, stdin);
		printf("\npasswd = %s\n", passwd);
		printf("Repeat your password: ");
		fgets(repeat, 100, stdin);
		printf("\nrepeat = %s\n", repeat);

		reset_input_mode();
		if (strcmp(passwd, repeat) == 0){
			// Passwords MATCH
			FILE * shadowptr = fopen(shadow, "w");
			// hash the password
			//gcry_md_open(GCRY_MD_SHA512, 1, )
			// fputs(hash, shadow);
			fclose(shadowptr);
		}
		else {
			// Passwords DO NOT MATCH
			exit(1);
		}
	}
}

void sanitize(const char * dir){
	// Remove the folder and any files inside.

	DIR * dirp = opendir(dir);
	struct dirent * entry;

	while ((entry = readdir(dirp)) != NULL){
		remove(entry->d_name);
	}
	closedir(dirp);
	rmdir(dir);
}

int main(int argc, const char * argv[]){
	// Main function.

	char * home = getenv("HOME");
	char dir[46] = { 0 };
	char shadow[55] = { 0 };
	char passes[55] = { 0 };

	strncpy(dir, home, strlen(home));
	strncat(dir, "/.pwmgr", strlen("/.pwmgr")+1);

	strncpy(shadow, dir, strlen(dir));
	strncat(shadow, "/shadow.pw", strlen("/shadow.pw")+1);

	strncpy(passes, dir, strlen(dir));
	strncat(passes, "/passes.pw", strlen("/passes.pw")+1);

	if (argc >= 2){
		// Check arguments
		if (strcmp(argv[1], "help") == 0 ||
			strcmp(argv[1], "h")    == 0){
			printf("%s", HELP);
		}
		else if (strcmp(argv[1], "initialize") == 0 || 
				 strcmp(argv[1], "i") 		   == 0){
			/* Should probably change this to look through all of the arguments
			   and tell if ANY of them equal "initialize" or "i" so we can do
			   position-independent flags. Same goes for the next checks. */
			initialize(dir, shadow, passes);
		}
		else if (strcmp(argv[1], "add") == 0 ||
				 strcmp(argv[1], "a")   == 0){
			printf("Not implemented yet.\n");
		}
		else if (strcmp(argv[1], "get") == 0 ||
				 strcmp(argv[1], "g")   == 0){
			printf("Not implemented yet.\n");
		}
		else if (strcmp(argv[1], "remove") == 0 ||
				 strcmp(argv[1], "rm")     == 0 ||
				 strcmp(argv[1], "r")      == 0){
			printf("Not implemented yet.\n");
		}
		else if (strcmp(argv[1], "list") == 0 ||
				 strcmp(argv[1], "ls")   == 0 ||
				 strcmp(argv[1], "l")    == 0){
			printf("Not implemented yet.\n");
		}
		else if (strcmp(argv[1], "sanitize") == 0){
			printf("Not implemented yet.\n");
		}
		else{
			printf("Argument not recognized: `%s`\n", argv[1]);
		}
	}
	else {
		printf("Not enough arguments.\n");
		printf("Try `pwmgr help`.\n");
	}

	return 0;
}
