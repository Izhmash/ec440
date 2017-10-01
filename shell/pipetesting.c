
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>


// 0 is stdin
// 1 is stdout

char *argv[2];

char *argv2[3];

void run_command(int pfd[])
{
    int pid;

    pid = fork();

    switch (pid) {
        case 0: //child
            dup2(pfd[0], 0);
            close(pfd[1]);
            execvp(argv2[0], argv2);
            perror(argv2[0]); 
        case -1:
            perror("fork");
            exit(1);
        default: //parent
            dup2(pfd[1], 1);
            close(pfd[0]);
            execvp(argv[0], argv);
            perror(argv[0]); 
    }
}

int main()
{
    argv[0] = "ls";
    argv[1] = NULL;

    argv2[0] = "wc";
    argv2[1] = NULL;
    argv2[2] = NULL;

    pid_t pid;
    int status;

    int fd[2];

    pipe(fd);

    pid = fork();

    switch (pid) {
        case 0: //child
            run_command(fd);
            exit(0);
        case -1:
            perror("fork");
            exit(1);
        default: //parent
            while ((pid = wait(&status)) != -1) {
                fprintf(stderr, "process %d exits with %d\n",
                        pid,
                        WEXITSTATUS(status));
                break;
            }
    }
    exit(0);

}
