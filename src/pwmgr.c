#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/random.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>

#define MAX_SZ 2097152 // 2 MiB maximum buffer for passwords

struct termios old, new;
unsigned char plain_contents[MAX_SZ] = {0};
unsigned char cipher_contents[MAX_SZ + 16] = {0}; // 2 MiB MAX_SZ + 16 B for new IV
unsigned char aes_key[32] = {0}; // 256-bit key

void reset_input_mode();
void set_input_mode();

int init();
int deinit(); // not finished yet
int check_is_initialized();

int load_file(); // not finished yet
int write_file(); // not finished yet

int encrypt_mem();
int decrypt_mem();

int get_pass(char * passwd, int len, int context);
int get_key(char * passwd, unsigned char * key);

int create_acct(); // not finished yet
int remove_acct(); // not finished yet
int show_all_acct(); // not finished yet
int get_acct(); // not finished yet

int main(int argc, char ** argv){
    umask(0077);

    if (check_is_initialized() != 0){
        printf("First use - initializing the pwfile.\n");
        init();
    }

    if (argc < 2){
        printf("Not enough arguments.\n");
        exit(1);
    }

    int c, index;
    char * acct_email = NULL;
    char * acct_user = NULL;
    char * acct_pass = NULL;

    opterr = 0;

    while ((c = getopt(argc, argv, "e:u:p:")) != -1){
        switch (c) {
            case 'e':
                acct_email = optarg;
                break;
            case 'u':
                acct_user = optarg;
                break;
            case 'p':
                acct_pass = optarg;
                break;
            case '?':
                if ((optopt == 'c') || (optopt == 'c') || (optopt == 'c')){
                    printf("Error: option -%c requires an argument.\n", optopt);
                }
                else if (isprint(optopt)){
                    printf("Error: unknown option '-%c'.\n", optopt);
                }
                else {
                    printf("Error: unknown option character '\\x%x'.\n", optopt);
                }
                return 1;
            default:
                abort();
        }
    }

    for (index = optind; index < argc; index++){
        if (strncmp(argv[index], "add", 3) == 0){
            printf("add\n");
            create_acct(acct_email, acct_user, acct_pass);
            break;
        }
        else if (strncmp(argv[index], "del", 3) == 0){
            printf("del\n");
            remove_acct();
            break;
        }
        else if (strncmp(argv[index], "list", 4) == 0){
            printf("list\n");
            show_all_acct();
            break;
        }
        else {
            printf("Usage:\n\
    %1$s add <service> [-e email] [-u username] [-p password]\n\
        add an account to the manager\n\
    %1$s del <service>\n\
        remove an account from the manager\n\
    %1$s list\n\
        list the accounts in the manager\n\
    %1$s help\n\
        show this message\n", argv[0]);
            break;
        }
    }

    memset(plain_contents, 0x00, MAX_SZ);
    memset(aes_key, 0x00, 32);
    return 0;
}

void reset_input_mode(){
    // Show characters on stdin again.

    tcsetattr(STDIN_FILENO, TCSANOW, &old);
}

void set_input_mode(){
    // Stop showing characters on stdin.

    if (!isatty (STDIN_FILENO)){
        fprintf(stderr, "Not a terminal.\n");
        exit(1);
    }

    if (tcgetattr(STDIN_FILENO, &old) != 0){
        printf("Error: can't set input mode\n");
        exit(1);
    }
    new = old;
    atexit(reset_input_mode);

    new.c_lflag &= ~ECHO;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &new);
}

int init(){
    // Create the pwmgr file

    if (check_is_initialized() == 0){
        char response[16] = {0};
        printf("The password file is already initialized.\n");
        printf("Do you want to reinitialize? [y/N] ");
        fgets(response, 16, stdin);
        fflush(stdin);

        if ((memcmp(response, "y", 1)) & (memcmp(response, "Y", 1))){
            printf("Not reinitializing.\n");
            exit(1);
        }

        deinit();
    }

    char * signature = "-PWMGR-\n";
    memcpy(plain_contents, signature, strlen(signature));

    // Ask the user for a password
    char passwd_1[256] = {0};
    char passwd_2[256] = {0};
    get_pass(passwd_1, 256, 0);
    get_pass(passwd_2, 256, 1);

    if (strncmp(passwd_1, passwd_2, 256) != 0){
        printf("Passwords don't match!\n");
        exit(1);
    }

    get_key(passwd_1, aes_key);
    memset(passwd_1, 0x00, 256);
    memset(passwd_2, 0x00, 256);

    // Encrypt plain_contents and write to file
    encrypt_mem();
    write_file();
}

int deinit(){
    // Delete the pwmgr file in preparation for uninstall

    char pwfile[256] = {0};
    strncpy(pwfile, getenv("HOME"), strlen(getenv("HOME")));
    strncat(pwfile, "/.pwmgr", 8);
    remove(pwfile);
}

int check_is_initialized(){
    // Check to see if the file exists, has the right permissions, etc

    char pwfile[256] = {0};
    strncpy(pwfile, getenv("HOME"), strlen(getenv("HOME")));
    strncat(pwfile, "/.pwmgr", 8);

    if (access(pwfile, F_OK) == -1){
        return 1;
    }

    struct stat pwfile_info;
    stat(pwfile, &pwfile_info);

    if (!(S_ISREG(pwfile_info.st_mode))){
        printf("Error: ~/.pwmgr is not a regular file.");
        exit(1);
    }

    if (!(pwfile_info.st_uid == getuid())){
        printf("Error: You do not own ~/.pwmgr.\n");
        exit(1);
    }

    if ((pwfile_info.st_mode & 0200) != 0200){
        printf("Error: You don't have write access on ~/.pwmgr.\n");
        exit(1);
    }

    if (pwfile_info.st_size != (MAX_SZ + 16)){
        printf("\nError: File is the wrong size and has been modified.\n\n");
        printf("We'll try to continue, but be aware you may have to\n");
        printf("delete and reinitialize the password file.\n\n");
    }

    if ((pwfile_info.st_mode & 0777) != 0600){
        printf("\nThe permissions on ~/.pwmgr are incorrect.\n\n");
        printf("Please be aware that someone may be able to modify\n");
        printf("or destroy data in your password file, preventing you\n");
        printf("from being able to recover your passwords.\n\n");
        printf("This could also cause issues writing the file.\n\n");
        printf("It is recommended that you run `chmod 600 ~/.pwmgr`.\n\n");
    }

    return 0;
}

int load_file(){
    // Load the contents of the file into cipher_contents

    char pwfile[256] = {0};
    strncpy(pwfile, getenv("HOME"), strlen(getenv("HOME")));
    strncat(pwfile, "/.pwmgr", 8);

    FILE * fptr = fopen(pwfile, "rb");
    if (fptr == NULL){
        perror("An error occurred while trying to load ~/.pwmgr:\n");
    }
    else {
        fread(cipher_contents, 1, (MAX_SZ + 16), fptr);
        fclose(fptr);
    }
}

int write_file(){
    // Write cipher_contents to the pwfile

    char pwfile[256] = {0};
    strncpy(pwfile, getenv("HOME"), strlen(getenv("HOME")));
    strncat(pwfile, "/.pwmgr", 8);

    FILE * fptr = fopen(pwfile, "wb");
    if (fptr == NULL){
        perror("An error occurred while trying to write ~/.pwmgr:\n");
    }
    else {
        fwrite(cipher_contents, 1, MAX_SZ + 16, fptr);
        fclose(fptr);
    }
}

int encrypt_mem(){
    // Encrypt the plaintext buf and write the result to the ciphertext buf

    unsigned char iv[16] = {0};
    unsigned char orig_iv[16] = {0};
    AES_KEY enc_key;

    // Generate some random bytes for the iv
    // Blocks if urandom is not initialized
    getrandom(iv, 16, 0);
    memcpy(orig_iv, iv, 16);

    AES_set_encrypt_key(aes_key, 256, &enc_key);
    AES_cbc_encrypt(plain_contents, cipher_contents, MAX_SZ, &enc_key, iv, AES_ENCRYPT);

    memcpy(&cipher_contents[MAX_SZ], orig_iv, 16);
    memset(plain_contents, 0x00, MAX_SZ);
}

int decrypt_mem(){
    // Decrypt the ciphertext buf and write the result to the plaintext buf

    unsigned char iv[16] = {0};
    memcpy(iv, &cipher_contents[MAX_SZ], 16);

    AES_KEY dec_key;

    AES_set_decrypt_key(aes_key, 256, &dec_key);
    AES_cbc_encrypt(cipher_contents, plain_contents, MAX_SZ, &dec_key, iv, AES_DECRYPT);
}

int get_pass(char * passwd, int len, int context){
    /*
     * Prompt the user for their password
     * Args:
     * char * passwd: buffer to write input into
     * int       len: length of buffer
     * int   context: first ask or subsequent ask (0 or >0)
     */

    if (context == 0){
        printf("Enter your password: ");
    }
    else {
        printf("Enter your password (again): ");
    }

    set_input_mode();
    fgets(passwd, len, stdin);
    reset_input_mode();
    printf("\n");

    return 0;
}

int get_key(char * passwd, unsigned char * key){
    /*
     * Turn a password into a usable AES256 key using HKDF and SHA256
     * Args:
     * char *       passwd: string to derive from
     * unsigned char * key: buffer to place derived key into
     */

    unsigned char salt[32] = {0};
    getrandom(salt, 32, 0);

    int N = 16384;
    int r = 8;
    int p = 8;  // must be less than (2^32 - 1) * hlen/MFlen
                  // hlen = 32 (32 bytes, 256 bits)
                  // MFlen = r * 128 per RFC7914

    int max_mem = (1024 * 1024 * 128); // 128 MiB

    EVP_PBE_scrypt(passwd, strlen(passwd), salt, sizeof(salt),
                   N, r, p, max_mem, key, 32);

    return 0;
}

int create_acct(char * email, char * user, char * pass){
    // Create an account and add it to the password file

    char acct_email[128];
    char acct_user[128];
    char acct_pass[128];

    if (!(email)){
        printf("Account email: ");
        fgets(acct_email, 128, stdin);
        printf("%s\n", acct_email);
    }
    else {
        strncpy(acct_user, user, 128);
        acct_user[127] = '\0';
    }
    if (!(user)){
        printf("Account username: ");
        fgets(acct_user, 128, stdin);
        printf("%s\n", acct_user);
    }
    else {
        strncpy(acct_user, user, 128);
        acct_user[127] = '\0';
    }
    if (!(pass)){
        printf("Account password: ");
        fgets(acct_pass, 128, stdin);
        printf("\n");
    }
    else {
        strncpy(acct_pass, pass, 128);
        acct_pass[127] = '\0';
    }

    // don't forget to zero out acct_pass and pass
}

int remove_acct(){
    //
}

int show_all_acct(){
    //
}

int get_acct(){
    //
}

