#include <myparser.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_BUFF_SIZE 512
#define MAX_TOKEN_SIZE 32
#define MAX_TOKENS 512


/*int main()
{
    int running = 1;
    char user_input[MAX_BUFF_SIZE];
    char tokens[MAX_TOKENS][MAX_TOKEN_SIZE];

    do {
        memset(user_input, 0, sizeof(user_input));
        memset(tokens, 0, sizeof(tokens[0][0]) * MAX_TOKENS * MAX_TOKEN_SIZE);

        int token_count = 0;
        int total_chars = 0;

        total_chars = fetch_input(user_input);
        token_count = get_tokens(total_chars, user_input, tokens);

        //printf ("%d\n", total_chars);
        //printf ("%d\n", token_count);
        
        //printf("%c\n", tokens[0][5]);
        //char *cmd = tokens[0];
        //char *argv[MAX_TOKENS];
        //argv[0] = cmd;
        //argv[1] = NULL;
        //execvp(cmd, argv);
        
        desc_tokens(tokens, token_count);
        
    } while (running);
    return 0;
}*/

// Returns number of chars in input
int fetch_input(char input[MAX_BUFF_SIZE])
{
    printf("%s", "my_parser> ");
    char *in = fgets(input, MAX_BUFF_SIZE, stdin);
    if (in == NULL)
        exit(0);
    return (unsigned)strlen(in) - 1;
}

// Returns number of tokens
// Parameters: input length, input array, token output 2D array
int get_tokens(int  input_chars,
               char input[MAX_BUFF_SIZE],
               char tokens[MAX_TOKENS][MAX_TOKEN_SIZE])
{
    int token_iter = 0;
    int token_char_iter = 0;
    const char *meta = "<>|&";
    int tc = 0;
    for (int i = 0; i < input_chars; ++i) {
        if (input[i] == ' ') {
            if (i > 0) {
                if (!strchr(meta, input[i - 1])) {
                    tokens[token_iter][token_char_iter + 1] = '\0';
                    token_iter++;
                }
            }
            token_char_iter = 0;
        } else if (strchr(meta, input[i])) { // Special character
            if (i > 0) {
                if (input[i - 1] != ' ' && !strchr(meta, input[i - 1])) {
                    tokens[token_iter][token_char_iter + 1] = '\0';
                    token_iter++;
                }
                token_char_iter = 0;
            }
            tokens[token_iter][token_char_iter] = input[i];
            tokens[token_iter][token_char_iter + 1] = '\0'; // Maybe not this one?
            tc++;
            token_iter++;
        } else {
            tokens[token_iter][token_char_iter] = input[i];
            token_char_iter++;
            if (i < input_chars) {
                if (input[i + 1] == ' ' ||
                        i == input_chars - 1 ||
                        strchr(meta, input[i + 1])) {
                    tc++;
                }
            }
        }
    }
    //tokens[tc - 1][0] = NULL;
    return tc;
}

// Lists tokens with descriptions
// Parameters: 2D array of tokens, number of tokens
void desc_tokens(char tokens[MAX_TOKENS][MAX_TOKEN_SIZE],
                 int  num_tokens)
{
   const char *meta = "<>|&";
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
   }
}
