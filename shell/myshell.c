#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <myparser.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

/*
 * Input: index to 1st command; # cmd toks, pointers to tok array + cmd array
 * Return: index to next meta character or -1 if none
 */
int make_args(int idx,
              int num_tokens,
              int *cmd_tokens,
              char *args[], 
              char *cmd[])
{
    int i;
    int meta_idx = -1;
    *cmd_tokens = 0;
    char *temp;
    char *meta = "<>|&"; 
    for (i = idx; i < num_tokens; ++i) {
        temp = strchr(meta, args[i][0]);
        if (temp && meta_idx == -1) {
            meta_idx = i;
            cmd[i - idx + 1] = NULL;
            return meta_idx;
        }

        cmd[i - idx] = args[i];
        (*cmd_tokens)++;

    }
    cmd[i - idx + 1] = NULL;
    return -1;
}

/*
 * Idea: Parse line, determine if there is 
 * at least one pipe.  Save all previous commands
 * into one array, then the rest into another.
 * After piping, then do the execution.
 */

int main(int argc, char **argv)
{
    int running = 1;
    char user_input[MAX_BUFF_SIZE];
    char tokens[MAX_TOKENS][MAX_TOKEN_SIZE];
    char *args[MAX_TOKENS+1];
    char *args2[MAX_TOKENS+1];
    char buffer[MAX_TOKENS];
    pid_t pid, pid2;

    do {
        char *cmd;
        int err;
        memset(user_input, 0, sizeof(user_input));
        memset(tokens, 0, sizeof(tokens[0][0]) * MAX_TOKENS * MAX_TOKEN_SIZE);
        memset(args, 0, sizeof(args[0]) * (MAX_TOKENS + 1));
        memset(args2, 0, sizeof(args2[0]) * (MAX_TOKENS + 1));
        memset(buffer, 0, sizeof(buffer[0]) * MAX_TOKENS + 1);

        int num_tokens = 0;
        int total_chars = 0;
        int cmd_tokens = 0;
        int num_pipes = 0;
        int num_inredirs = 0;
        int num_outredirs = 0;
        int background = 0;

        total_chars = fetch_input(user_input, argc);
        if (total_chars == -1) {
            printf("Error: input too long.\n");
            continue;
        }
        num_tokens = get_tokens(total_chars, user_input, tokens);
        if (num_tokens == 0) {
            continue;
        }

        // Build arg string array and get pipe info
        int j;
        for (j = 0; j < num_tokens; ++j) {
            args[j] = tokens[j];
            if (args[j][0] == '|') {
                num_pipes++;
            } else if (args[j][0] == '<') {
                num_inredirs++;
            } else if (args[j][0] == '>') {
                num_outredirs++;
            }
        }
        if (num_inredirs > 1 || num_outredirs > 1) {
            printf("Error: too many redirections\n");
            continue;
        }
        //printf("%d\n%d\n", num_redirs, num_pipes);
        args[num_tokens] = NULL;

        char *meta = "<>|&"; 
        // Check for first instance of meta character
        
        int i;
        //int meta_idx = -1;
        char *temp;
        int num_tokens2 = 0;
        char cur_meta = ' ';
        //temp = strchr(meta, args[0][0]);
        //printf("%d\n", ptr - meta);
        
        // Each child carries the rest of the args
        // and spawns a new child if there is a pipe?

        for (i = 0; i < num_tokens - 1; ++i) {
            if (args[i][0] == '&') {
                printf("Error: & must be at end of line\n");
                continue;
            }
        }
        if (args[num_tokens - 1][0] == '&') {
            background = 1;
        }
        // Redirection
        
        //TODO Add check for "<<" or ">>"
        int out, out_orig, in, in_orig;

        // Pipe making, child making, args and executing
        //pid_t pid, pid2;
        //int status;
        int hfd[2];
        //int pipefds[num_pipes * 2];
        int pipefd[2];
        int r_pipe[2];
        pipe(hfd); // To aid with errored children
        // Make them pipes
        pipe(pipefd);
        pipe(r_pipe);
        fcntl(hfd[1], F_SETFD, fcntl(hfd[1], F_GETFD) | FD_CLOEXEC);

        //cmd = args[0]; // Need to address

        int meta_idx = -1;
        int cmd_idx = 0; 
        int ccounter = 0;
        int read_size = 0;
        in_orig = dup(fileno(stdin));
        out_orig = dup(fileno(stdout));

        
        //for (i = 0; i < num_pipes + 1; ++i) {
            meta_idx = make_args(cmd_idx, num_tokens, &cmd_tokens, args, args2);
            cmd = args2[0];
            //printf("%d\n", cmd_tokens);
            args2[cmd_tokens] = NULL; //Hacky fix
            cmd_idx = meta_idx + 1;
            // Redirection in and out
            if (num_tokens > 3 && args[meta_idx][0] == '<' && args[num_tokens - 2][0] == '>') {
                out = open(args[num_tokens - 1], O_RDWR|O_CREAT|O_APPEND, 0600);
                out_orig = dup(fileno(stdout));
                if ((in = open(args[cmd_tokens + 1], 
                                O_RDONLY, 
                                S_IRUSR | S_IRGRP | S_IROTH
                                )) == -1) {
                    printf("Error: file %s not found.\n", args2[cmd_tokens]); 
                    continue;
                }
                in_orig = dup(fileno(stdin));
                if (dup2(out, fileno(stdout)) == -1) { 
                    printf("Error: cannot redirect stdout.\n");
                    continue;
                }
                if (dup2(in, fileno(stdin)) == -1) { 
                    printf("Error: cannot redirect stdin.\n"); 
                    continue;
                }
            // Redirect output
            } else if (num_outredirs && args[meta_idx][0] == '>') {
                out = open(args[meta_idx + 1], O_RDWR|O_CREAT|O_APPEND, 0600);
                out_orig = dup(fileno(stdout));

                if (dup2(out, fileno(stdout)) == -1) { 
                    printf("Error: cannot redirect stdout.\n");
                    continue;
                }
             // Redirect input
            } else if (num_inredirs && args[meta_idx][0] == '<') {
                if ((in = open(args[cmd_tokens + 1], 
                                O_RDONLY, 
                                S_IRUSR | S_IRGRP | S_IROTH
                                )) == -1) {
                    printf("Error: file %s not found.\n", args2[cmd_tokens]); 
                    continue;
                }
                in_orig = dup(fileno(stdin));

                if (dup2(in, fileno(stdin)) == -1) { 
                    printf("Error: cannot redirect stdin.\n"); 
                    continue;
                }
            }
            // Shortcoming: Hard-coded to allow for one pipe w/ two commands.
            pid = fork();
            if (pid == 0) {
                close(hfd[0]);
                if (num_pipes > 0) { // child 1
                    pid2 = fork();
                    close(hfd[0]);
                    if (pid2 == 0) { //child 2
                        dup2(pipefd[0], 0);
                        //dup2(r_pipe[1], fileno(stdout));
                        close(pipefd[1]);
                        //close(r_pipe[0]);
                        meta_idx = make_args(meta_idx + 1, num_tokens, &cmd_tokens, args, args2);
                        cmd = args2[0];
                        args2[cmd_tokens] = NULL; //Hacky fix
                        if (execvp(cmd, args2) == -1) {
                            printf("Error: command not found.\n");
                        }
                        write(hfd[1], &errno, sizeof(int));
                        _exit(0);
                    } else if (pid2 > 0) { //parent 2
                        dup2(pipefd[1], 1);
                        close(pipefd[0]);
                        if (execvp(cmd, args2) == -1) {
                            printf("Error: command not found.\n");
                        }
                        while (waitpid(pid2, &err, 0) == -1) {
                            if (WIFEXITED(err)) {
                                printf("Child process exited with %d status\n",
                                        WEXITSTATUS(err));
                            }
                            if (errno != EINTR) {
                                printf("Error: waitpid.\n");
                                break;
                            }
                        }
                        write(hfd[1], &errno, sizeof(int));
                        _exit(0);
                        
                    }
                    write(hfd[1], &errno, sizeof(int));
                    _exit(0);
                } else { // No pipe
                    if (execvp(cmd, args2) == -1) {
                        printf("Error: command not found.\n");
                    }
                    write(hfd[1], &errno, sizeof(int));
                    _exit(0);
                }
            } else { //parent 1
                // Close pipe
                close(hfd[1]);
                while ((read(hfd[0], &err, sizeof(errno))) == -1) {
                    if (errno != EAGAIN && errno != EINTR) break;
                }
                // Do more stuff?

                //read_size = read(r_pipe[0], buffer, MAX_TOKENS);
                close(hfd[0]);
                fflush(stdout);
                fflush(stdin);
                if (out > 2) { // 0 stdin, 1 stdout, 2 stderr
                    close(out);
                }
                if (in > 2) {
                    close(in);
                }
                //dup2(out_orig, fileno(stdout));
                //dup2(in_orig, fileno(stdin));
                dup2(0, pipefd[0]);
                dup2(1, pipefd[1]);
                close(pipefd[0]);
                //printf("%s", buffer);
                if (!background) {
                    while (waitpid(pid, &err, 0) == -1) {
                        if (WIFEXITED(err)) {
                            printf("Child process exited with %d status\n",
                                    WEXITSTATUS(err));
                        }
                        if (errno != EINTR) {
                            printf("Error: waitpid.\n");
                            break;
                        }
                    }
                }
            }
        waitpid(-1, NULL, WNOHANG); // Cleanup zombie children
        //dup2(in_orig, pipefd[0]);
        //dup2(out_orig, pipefd[1]);
        dup2(in_orig, fileno(stdin));
        dup2(out_orig, fileno(stdout));
        //close(in_orig);
        //close(out_orig);
        
    } while (running);
    kill(pid, SIGKILL);
    kill(pid2, SIGKILL);
    return 0;
}
