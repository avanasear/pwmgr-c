#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gcrypt.h>
//#include <json/json.h>
#include <termios.h>

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

struct termios saved_attributes;

void reset_input_mode(){
	tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}

void set_input_mode(){
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
		//printf("\npasswd = %s\n", passwd);
		printf("Repeat your password: ");
		fgets(repeat, 100, stdin);
		//printf("\nrepeat = %s\n", repeat);

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

int main(int argc, const char * argv[]){
	// Main function.

	char * home = getenv("HOME");
	char dir[46] = { 0 };
	char shadow[55] = { 0 };
	char passes[55] = { 0 };

	strncpy(dir, home, strlen(home));
	strncat(dir, "/.pwmgr", strlen("/.pwmgr"));

	strncpy(shadow, dir, strlen(dir));
	strncat(shadow, "/shadow.pw", strlen("/shadow.pw"));

	strncpy(passes, dir, strlen(dir));
	strncat(passes, "/passes.pw", strlen("/passes.pw"));

	//printf("%d\n", is_initialized(dir, shadow, passes));
	initialize(dir, shadow, passes);
	//printf("%s\n%s\n", home, dir);
	//printf("%d\n", is_initialized(dir, shadow, passes));

	return 0;
}
