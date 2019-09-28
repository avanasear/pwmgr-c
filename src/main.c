#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gcrypt.h>

int is_initialized(const char * dir, const char * shadow, const char * passes){
	/* Function to tell if:
	    a. The directory exists and
	    b. The files exist */

	if (access(dir, F_OK) != -1){
		if (access(shadow, F_OK) != -1){
			if (access(passes, F_OK) != -1){
				return 0;
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

int main(int argc, const char * argv[]){
	// Main function.

	char * user = getenv("USER");
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

	printf("%s\n%s\n%s\n", user, home, dir);
	printf("%d\n", is_initialized(dir, shadow, passes));

	return 0;
}