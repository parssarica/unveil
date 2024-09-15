#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <glib.h>
#include "queuesage.h"

#define SYS_openat 257 // The syscall number for openat on x86_64

int relative2absolute(char *relative_path, char **absolute)
{
    char absolute_path[PATH_MAX];

    if (realpath(relative_path, absolute_path) == NULL)
    {
        perror("realpath");
        return EXIT_FAILURE;
    }

    strlcpy(*absolute, absolute_path, PATH_MAX);
    return EXIT_SUCCESS;
}

void print_hash_table_keys(GHashTable *hash_table) {
    GHashTableIter iter;
    gpointer key, value;

    // Initialize the iterator
    g_hash_table_iter_init(&iter, hash_table);

    // Iterate over the hash table and print keys
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        printf("Key: %s\n", (char *)key);
    }
}

msg message;
unsigned int priority;
char* msg_content;
//char unveil_content[256];
int unveil_available = 1;

void print_flags(int flags, char** flaglist)
{
    if(flags & O_RDONLY) strlcpy(*flaglist, "r", sizeof(flaglist));
    if(flags & O_WRONLY) strlcpy(*flaglist, "w", sizeof(flaglist));
    if(flags & O_RDWR) strlcpy(*flaglist, "rw", sizeof(flaglist));
    if(flags & O_APPEND) strlcpy(*flaglist, "a", sizeof(flaglist));
    if(flags & O_CREAT) strlcpy(*flaglist, "c", sizeof(flaglist));
    if(flags & O_TRUNC) strlcpy(*flaglist, "t", sizeof(flaglist));
    if(flags & O_DIRECTORY) strlcpy(*flaglist, "D", sizeof(flaglist));
    if(flags & O_CLOEXEC) strlcpy(*flaglist, "C", sizeof(flaglist));
}

void get_filename(pid_t child, long addr, char *filename, size_t size) {
    for (size_t i = 0; i < size; i += sizeof(long)) {
        errno = 0;
        long word = ptrace(PTRACE_PEEKDATA, child, addr + i, NULL);
        if (word == -1) {
            if (errno == 0) {
                // If errno is 0, it means we reached the end of the string (NULL terminator)
                break;
            } else {
                perror("ptrace PEEKDATA");
                break;
            }
        }
        memcpy(filename + i, &word, sizeof(long));
    }
}

void receiver(GHashTable *unveil_list)
{
    char* token;
    char file[512];
    char mode[8];
    int i;

    if(receive(&message, &priority, &msg_content) > 0)
    {
        printf("msg_content:<%s>\n", msg_content);
        if(strlen(msg_content) <= 0) return;
        if(unveil_available && strcmp(msg_content, "unveil_ended")==0)
        {
            printf("unveil_ended\n");
            unveil_available = 0;
            return;
        }
        else if(unveil_available)
        {
            token = strtok(msg_content, "|");
            i = 0;
            file[0] = 0;
            mode[0] = 0;
            while(token != NULL)
            {
                if(i++ == 0)
                {
                    strlcpy(file, token, sizeof(file)-1);
                    printf("file: <%s>\n", file);
                }
                else
                {
                    strlcpy(mode, token, sizeof(mode)-1);
                    printf("mode: <%s>\n", mode);
                }
                token = strtok(NULL, "|");
            }
            printf("completed receiving msg: <%s>\n", file);
            if(strlen(file) > 0 && strlen(mode) > 0)
            {
                g_hash_table_insert(unveil_list, g_strdup(file), g_strdup(mode));
                printf("Added to list: %s %s\n", file, mode);
            }
            //strlcpy(unveil_content, msg_content, sizeof(unveil_content)-1);
        }
        else
        {
            if(!unveil_available) printf("Caution: Unveil available disabled.\n");
        }
    }
    return;
}

void trace_child(const char *program) {
    pid_t child = fork();
    int status;
    struct user_regs_struct regs;
    char filename[256];
    char filename_trace[1024]={0};
    long data;
    unsigned long i;
    size_t filename_size = 256;
    int found;
    long filename_addr;
    long flags;
    GHashTableIter iter;
    gpointer file_ptr, mode_ptr;
    GHashTable *unveil_list = g_hash_table_new(g_str_hash, g_str_equal);
    char *absolute1;
    char *absolute2;
    absolute1 = malloc(PATH_MAX);
    absolute2 = malloc(PATH_MAX);

    if(setup(&message, "/queuesage1") == (mqd_t)-1)
    {
        perror("setup quesage\n");
        exit(1);
    }


    if (child == 0) {
        // Child process
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execl(program, program, NULL);
        perror("execl failed\n");
        exit(1);
    } else {
        // Parent process
        while (1) {
            // Yeni systemcall
            waitpid(child, &status, 0);
            if (WIFEXITED(status)) {
                break;
            }

            if(unveil_available)
            {
                receiver(unveil_list);
            }

            // Get the registers
            ptrace(PTRACE_GETREGS, child, NULL, &regs);

            // Check if the syscall is openat
            if (regs.orig_rax == SYS_openat && (!unveil_available || g_hash_table_size(unveil_list))) {
                // Systemcall openat ise
                // Log the syscall
                printf("openat called with dfd: %lld\n", regs.rdi);
                printf("openat called with dfd: %lld\n", regs.rsi);

                // Read the filename argument

                // Read the filename from the child's memory
                filename_addr = regs.rsi;       /*birinci parametre rdi, ikinci parametre rsi. open'da birinci parametrede filename, openat'de ikinci parametrede rsi. thats why rsi aldik.*/
                flags = regs.rdx;
                get_filename(child, filename_addr, filename, filename_size);
                relative2absolute(filename, &absolute1);
                printf("From syscall filename: <%s> -> <%s>\n", filename, absolute1);

                print_hash_table_keys(unveil_list);

                g_hash_table_iter_init(&iter, unveil_list);
                found = 0;
                while(g_hash_table_iter_next(&iter, &file_ptr, &mode_ptr))
                {
                    printf("\n-----sys------compare------list----------\n<%s><%s>\n", absolute1, (char*)file_ptr);
                    if(strcmp(absolute1, (char*)file_ptr) == 0)
                    {
                        found = 1;
                        printf("Found!!!\n");
                    }
                }

                if(!found)
                {
                    printf("Not found in the list!\n");
                    ptrace(PTRACE_DETACH, child, NULL, NULL);
                    kill(child, SIGKILL);
                    waitpid(child, NULL, 0);
                    printf("T800: Hasta la vista baby.\n");
                    break;
                }
                else
                {
                    printf("Found in the list!\n");
                }
            }

            // Continue the child process
            ptrace(PTRACE_SYSCALL, child, NULL, NULL);
        }

        terminate_queue(&message);
        free(msg_content);
        free(absolute1);
        free(absolute2);
        g_hash_table_destroy(unveil_list);
    }
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program_to_trace>\n", argv[0]);
        return 1;
    }

    trace_child(argv[1]);
    return 0;
}
