#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
    char *cmd = "ls";
    char *argv[4];
    argv[0] = "ls";
    argv[1] = "-a";
    argv[2] = "-l";
    argv[3] = NULL;

    pid_t pid;
    if (pid = fork() == 0) {
        execvp(cmd, argv);
    }
    printf("The child has exec'd the command\n");
}
