#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <myparser.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

/*
 * Idea: Parse line, determine if there is 
 * at least one pipe.  Save all previous commands
 * into one array, then the rest into another.
 * After piping, then do the execution.
 */

int main(int argc, char **argv)
{
    int running = 1;
    int background = 0;
    int err;
    char user_input[MAX_BUFF_SIZE];
    char tokens[MAX_TOKENS][MAX_TOKEN_SIZE];
    char *cmd;
    char *args[MAX_TOKENS+1];
    char *args2[MAX_TOKENS+1];

    do {
        memset(user_input, 0, sizeof(user_input));
        memset(tokens, 0, sizeof(tokens[0][0]) * MAX_TOKENS * MAX_TOKEN_SIZE);

        int num_tokens = 0;
        int total_chars = 0;
        int num_pipes = 0;
        int num_redirs = 0;

        total_chars = fetch_input(user_input, argc);
        if (total_chars == -1) {
            printf("Error: input too long.\n");
            continue;
        }
        num_tokens = get_tokens(total_chars, user_input, tokens);

        //printf ("%d\n", total_chars);
        //printf ("%d\n", num_tokens);
        
        // Build arg string array and get pipe info
        int j;
        for (j = 0; j < num_tokens; ++j) {
            args[j] = tokens[j];
            if (args[j][0] == '|') {
                num_pipes++;
            } else if (args[j][0] == '<' || args[j][0] == '>') {
                num_redirs++;
            }
        }
        //printf("%d\n%d\n", num_redirs, num_pipes);
        args[num_tokens] = NULL;

        const char *meta = "<>|&"; 
        // Check for first instance of meta character
        
        int i;
        int meta_idx = -1;
        char *temp;
        int num_tokens2 = 0;
        char cur_meta = ' ';
        //temp = strchr(meta, args[0][0]);
        //printf("%d\n", ptr - meta);
        
        // Each child carries the rest of the args
        // and spawns a new child if there is a pipe?

        for (i = 0; i < num_tokens; ++i) {
            if (args[i][0] == '&') {
                background = 1;
            }
            temp = strchr(meta, args[i][0]);
            if (temp && meta_idx == -1) {
                meta_idx = i;
                cur_meta = args[meta_idx][0];
                continue;
            }

            if (meta_idx != -1) {
                args2[i - meta_idx - 1] = args[i];
                num_tokens2++;
            }
        }
        args[meta_idx] = NULL; // Cap the args array
        
        // Redirection
        
        //TODO Add check for "<<" or ">>"
        int out, out_orig, in, in_orig;
        if (cur_meta == '<') { //TODO Need to check if there is an arg after
            if ((in = open(args2[0], 
                            O_RDONLY, 
                            S_IRUSR | S_IRGRP | S_IROTH
                            )) == -1) {
                printf("Error: file %s not found.\n", args2[0]); 
                continue;
            }
            in_orig = dup(fileno(stdin));

            if (dup2(in, fileno(stdin)) == -1) { 
                printf("Error: cannot redirect stdin.\n"); 
                continue;
            }
        } else if (cur_meta == '>') {
            out = open(args2[2], O_RDWR|O_CREAT|O_APPEND, 0600);
            out_orig = dup(fileno(stdout));

            if (dup2(out, fileno(stdout)) == -1) { 
                printf("Error: cannot redirect stdout.\n");
                continue;
            }
        } //else if (cur_meta == '|') {

        //}

        // Pipe making, child making, args and executing
        //TODO support for multiple commands
        pid_t pid;
        //int status;
        int hfd[2];
        int pipefds[num_pipes * 2];
        pipe(hfd); // To aid with errored children
        // Make them pipes
        for (i = 0; i < num_pipes; ++i) {
            pipe(pipefds + (i * 2));
        }
        fcntl(hfd[1], F_SETFD, fcntl(hfd[1], F_GETFD) | FD_CLOEXEC);

        cmd = args[0]; // Need to address

        if ((pid = fork()) == 0) {
            close(hfd[0]);
            if (execvp(cmd, args) == -1) {
                printf("Error: command not found");
            }
            write(hfd[1], &errno, sizeof(int));
            _exit(0);
        } else {
            // Close pipe
            close(hfd[1]);
            while ((read(hfd[0], &err, sizeof(errno))) == -1) {
                if (errno != EAGAIN && errno != EINTR) break;
            }
            // Do more stuff?
            close(hfd[0]);
            fflush(stdout);
            if (out > 2) { // 0 stdin, 1 stdout, 2 stderr
                close(out);
            }
            if (in > 2) {
                close(in);
            }
            dup2(out_orig, fileno(stdout));
            dup2(in_orig, fileno(stdin));
            if (!background) {
                while (waitpid(pid, &err, 0) == -1) {
                    /*if (WIFEXITED(err)) {
                        printf("Child process exited with %d status\n",
                                WEXITSTATUS(err));
                    }*/
                    if (errno != EINTR) {
                        printf("Error: waitpip");
                    }
                }
                /*if (WIFEXITED(err)) {
                    printf("child exited with %d\n", WEXITSTATUS(err));
                } else if (WIFSIGNALED(err)) {
                    printf("child killed by %d\n", WTERMSIG(err));
                }*/
            }
        }
        
    } while (running);
    return 0;
}
