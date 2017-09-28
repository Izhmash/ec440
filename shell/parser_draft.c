#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <myparser.h>
#include <string.h>
#include <fcntl.h>

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

        total_chars = fetch_input(user_input, argc);
        num_tokens = get_tokens(total_chars, user_input, tokens);

        //printf ("%d\n", total_chars);
        //printf ("%d\n", num_tokens);
        
        // Build arg string array
        int j;
        for (j = 0; j < num_tokens; ++j) {
            args[j] = tokens[j];
            //printf("%c\n", args[j][0]);
        }
        args[num_tokens] = NULL;

        const char *meta = "<>|&"; 
        // Check for first instance of meta character
        
        int i;
        int meta_idx = -1;
        char *temp;
        int num_tokens2 = 0;
        char cur_meta;
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
        //printf("%d\n", meta_idx); //Works!
        //printf("%d\n", num_tokens2); 
        //printf("%s\n", args[meta_idx]); //Works!
        //printf("%s\n", args2[0]); //Works!
        //printf("%c\n", cur_meta); //Works!
        
        // Redirection
        
        int out, out_orig;
        if (cur_meta == '<') {
            printf("Redirect input\n");
        } else if (cur_meta == '>') {
            printf("Redirect output\n");
            int out = open(args2[0], O_RDWR|O_CREAT|O_APPEND, 0600);
            int out_orig = dup(fileno(stdout));

            if (dup2(out, fileno(stdout)) == -1) { 
                perror("cannot redirect stdout"); 
                return 255; 
            }
        }

        // Testing exec and arg building
        pid_t pid;
        int status;

        cmd = args[0];
        if ((pid = fork()) == 0) {
            execvp(cmd, args);
        } else {
            // Close pipe
            fflush(stdout);
            if (out > 2) { // 0 stdin, 1 stdout, 2 stderr
                close(out);
            }
            dup2(out_orig, fileno(stdout));
            if (!background) {
                if (wait(&status) > 0) {
                    if (WIFEXITED(status)) {
                        printf("Child process exited with %d status\n",
                                WEXITSTATUS(status));
                    }
                }
            }
        }
        
        //desc_tokens(tokens, token_count);
        /*const char *meta = "<>|&";
        int last_state = 0; // 0: command, 1: arg, 2: pipe, 3: other meta
        
        for (int i = 0; i < num_tokens; ++i) {
            printf("%s - ", tokens[i]);
            if (i == 0 || last_state == 2) {
                last_state = 0;
                printf("command\n");
            } else if (strchr(meta, tokens[i][0])) {
                if (tokens[i][0] == '|') {
                    last_state = 2;
                    printf("pipe\n");
                }
                else {
                    last_state = 3;
                    if (tokens[i][0] == '<') {
                        printf("input redirect\n");
                    } else if (tokens[i][0] == '>') {
                        printf("output redirect\n");
                    } else {
                        printf("background\n");
                    }
                }
            } else {
                last_state = 1;
                printf("argument\n");
            }
        }*/
        
    } while (running);
    return 0;
}
