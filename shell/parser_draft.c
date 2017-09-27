#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <myparser.h>
#include <string.h>

/*
 * Idea: Parse line, determine if there is 
 * at least one pipe.  Save all previous commands
 * into one array, then the rest into another.
 * After piping, then do the execution.
 */

int main()
{
    int running = 1;
    char user_input[MAX_BUFF_SIZE];
    char tokens[MAX_TOKENS][MAX_TOKEN_SIZE];
    char *cmd;
    char *argv[MAX_TOKENS+1];
    char *argv2[MAX_TOKENS+1];

    do {
        memset(user_input, 0, sizeof(user_input));
        memset(tokens, 0, sizeof(tokens[0][0]) * MAX_TOKENS * MAX_TOKEN_SIZE);

        int num_tokens = 0;
        int total_chars = 0;

        total_chars = fetch_input(user_input);
        num_tokens = get_tokens(total_chars, user_input, tokens);

        //printf ("%d\n", total_chars);
        printf ("%d\n", num_tokens);
        
        // Build arg string array
        int j;
        for (j = 0; j < num_tokens; ++j) {
            argv[j] = tokens[j];
        }
        argv[num_tokens] = NULL;

        const char *meta = "<>|&"; 
        // Check for first instance of meta character
        

        /*int i;
        int meta_idx = -1;
        for (i = 0; i < num_tokens; ++meta_idx) {
            if (strchr(meta, tokens[0][i]) && meta_idx != -1) {
                meta_idx = i;
            }

            if (meta_idx != -1) {
                argv2[i - meta_idx] = tokens[i];
            }
        }*/

        // Testing exec and arg building
        cmd = argv[0];
        execvp(cmd, argv);

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
