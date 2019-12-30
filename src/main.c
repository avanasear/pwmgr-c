#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/random.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>

struct termios old, new;
unsigned char plain_contents[2097152] = {0}; // 2 MiB maximum size
unsigned char cipher_contents[2097168] = {0}; // 2 MiB maximum size + 16 B for new IV
unsigned char aes_key[32] = {0}; // 256-bit key

void reset_input_mode();
void set_input_mode();

int init(); // not finished yet
int deinit(); // not finished yet
int is_initialized(); // not finished yet

int load_file(); // not finished yet
int write_file(); // not finished yet

int encrypt_mem(); // not finished yet
int decrypt_mem(); // not finished yet

int get_pass(char * passwd, int len, int context);
int get_key(char * passwd, unsigned char * key); // not finished yet

int main(){
    char * user = getenv("USER");
    char * homedir = getenv("HOME");
    printf("You are %s.\nYour home directory is %s.\n", user, homedir);

    deinit();
    init();

    memset(plain_contents, 0x00, 2097152);
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
    tcgetattr(STDIN_FILENO, &old);
    atexit(reset_input_mode);
    tcgetattr(STDIN_FILENO, &new);
    new.c_lflag &= ~(ICANON | ECHO);
    new.c_cc[VMIN] = 1;
    new.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &new);
}

int init(){
    // Create the pwmgr file
    char * signature = "-PWMGR-\n";
    memcpy(plain_contents, signature, strlen(signature));

    // Ask the user for a password
    char passwd_1[256] = {0};
    char passwd_2[256] = {0};
    get_pass(passwd_1, 256, 0);
    get_pass(passwd_2, 256, 1);

    if (strncmp(passwd_1, passwd_2, 256) != 0){
        perror("Passwords don't match!\n");
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

int is_initialized(){
    // Check to see if the file exists already
    char pwfile[256] = {0};
    strncpy(pwfile, getenv("HOME"), strlen(getenv("HOME")));
    strncat(pwfile, "/.pwmgr", 8);

    struct stat pwfile_info;
    switch (pwfile_info.st_mode & S_IFMT) {
        case S_IFBLK:
            perror("Error: ~/.pwmgr is not a regular file (block device).\n");
            exit(1);
        case S_IFCHR:
            perror("Error: ~/.pwmgr is not a regular file (character device).\n");
            exit(1);
        case S_IFDIR:
            perror("Error: ~/.pwmgr is not a regular file (directory).\n");
            exit(1);
        case S_IFIFO:
            perror("Error: ~/.pwmgr is not a regular file (FIFO/pipe).\n");
            exit(1);
        case S_IFLNK:
            perror("Error: ~/.pwmgr is not a regular file (symlink).\n");
            exit(1);
        case S_IFREG:
            break;
        case S_IFSOCK:
            perror("Error: ~/.pwmgr is not a regular file (socket).\n");
            exit(1);
        default:
            perror("Error: ~/.pwmgr is an unknown filetype.\n");
            exit(1);
    }

    if (pwfile_info.st_mode != 0600){
        printf("The permissions on ~/.pwmgr are incorrect.\n\n");
        printf("Please be aware that someone may be able to modify\n");
        printf("or destroy data in your password file, preventing you\n");
        printf("from being able to recover your passwords.\n\n");
        printf("It is recommended that you run `chmod 600 ~/.pwmgr`.\n");
    }
    return 0;
}

int load_file(){
    // Load the contents of the file into cipher_contents
    char pwfile[256] = {0};
    strncpy(pwfile, getenv("HOME"), strlen(getenv("HOME")));
    strncat(pwfile, "/.pwmgr", 8);

    FILE * fptr = fopen(pwfile, "rb");
    fread(cipher_contents, 1, 2097168, fptr);
    fclose(fptr);
}

int write_file(){
    // Write cipher_contents to the pwfile
    char pwfile[256] = {0};
    strncpy(pwfile, getenv("HOME"), strlen(getenv("HOME")));
    strncat(pwfile, "/.pwmgr", 8);

    FILE * fptr = fopen(pwfile, "wb");
    fwrite(cipher_contents, 1, 2097168, fptr);
    fclose(fptr);
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
    AES_cbc_encrypt(plain_contents, cipher_contents, 2097152, &enc_key, iv, AES_ENCRYPT);

    memcpy(&cipher_contents[2097152], orig_iv, 16);
    memset(plain_contents, 0x00, 2097152);
}

int decrypt_mem(){
    // Decrypt the ciphertext buf and write the result to the plaintext buf
    unsigned char iv[16] = {0};
    memcpy(iv, &cipher_contents[2097152], 16);

    AES_KEY dec_key;

    AES_set_decrypt_key(aes_key, 256, &dec_key);
    AES_cbc_encrypt(cipher_contents, plain_contents, 2097152, &dec_key, iv, AES_DECRYPT);
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

    int N = 1024;
    int r = 8;
    int p = 128;  // must be less than (2^32 - 1) * hlen/MFlen
                  // hlen = 32 (32 bytes, 256 bits)
                  // MFlen = r * 128 per RFC7914

    int max_mem = (1024 * 1024 * 128); // 128 MiB

    EVP_PBE_scrypt(passwd, strlen(passwd), salt, sizeof(salt),
                   N, r, p, max_mem, key, 32);
}

