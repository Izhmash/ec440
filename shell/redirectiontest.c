#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>


// 0 is stdin
// 1 is stdout

int main()
{
    // Redirection
    int out = open("cout.log", O_RDWR|O_CREAT|O_APPEND, 0600);
    int out_orig = dup(fileno(stdout));

    if (dup2(out, fileno(stdout)) == -1) { 
        perror("cannot redirect stdout"); 
        return 255; 
    }

    
    // Exec command
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
   
    // Cleanup redirection 
    fflush(stdout); 
    close(out);
    dup2(out_orig, fileno(stdout));
    close(out_orig);
}
