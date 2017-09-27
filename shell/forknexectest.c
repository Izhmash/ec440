#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main()
{
    char *cmd = "ls";
    char *argv[4];
    argv[0] = "ls";
    argv[1] = "-a";
    argv[2] = "-l";
    argv[3] = NULL;

    pid_t pid;
    int status;
    if (pid = fork() == 0) {
        execvp(cmd, argv);
    } else {
        if (wait(&status) >= 0) {
            if (WIFEXITED(status)) {
                printf("Child process exited with %d status\n", 
                        WEXITSTATUS(status));
            }
        }
    }
    printf("The child has exec'd the command\n");
}
