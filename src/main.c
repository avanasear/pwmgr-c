#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gcrypt.h>
//#include <json/json.h>

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
	char * passwd;
	char * repeat;

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
		// REPLACE. THIS. WITH. SOMETHING. ELSE. USE TERMIOS OR SOMETHING.
		passwd = getpass("Create a password: ");
		repeat = getpass("Confirm your password: ");
		if (strcmp(passwd, repeat) == 0){
			// Passwords MATCH
			FILE * shadowptr = fopen(shadow, "w");
			// hash the password
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