#include <stdio.h>
#include <string.h>

#define MAX_BUFF_SIZE 512
#define MAX_TOKEN_SIZE 32
#define MAX_TOKENS 512

int fetch_input(char input[MAX_BUFF_SIZE]);
int get_tokens(int input_chars, char input[MAX_BUFF_SIZE], char tokens[MAX_TOKENS][MAX_TOKEN_SIZE]);

int main()
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

        printf ("%d\n", total_chars);
        printf ("%d\n", token_count);

        for(int i = 0; i <= token_count; ++i) {
            printf("%s\n", tokens[i]);
        }
        
    } while (running);
    return 0;
}

// Returns number of chars in input
int fetch_input(char input[MAX_BUFF_SIZE])
{
    printf("%s", "my_parser> ");
    char *in = fgets(input, MAX_BUFF_SIZE, stdin);
    return (unsigned)strlen(in) - 1;
}

// Returns number of tokens
// Parameters: input length, input array, token output 2D array
int get_tokens(int input_chars, char input[MAX_BUFF_SIZE], char tokens[MAX_TOKENS][MAX_TOKEN_SIZE])
{
    int token_iter = 0;
    int token_char_iter = 0;
    const char *meta = "<>|&";
    int tc = 0;
    for (int i = 0; i < input_chars; ++i) {
        if (input[i] == ' ') {
            token_char_iter = 0;
            if (i > 0) {
                if (!strchr(meta, input[i - 1])) {
                    token_iter++;
                }
            }
        } else if (strchr(meta, input[i])) { // Special character
            token_char_iter = 0;
            if (i > 0) {
                if (input[i - 1] != ' ' && !strchr(meta, input[i - 1])) {
                    token_iter++;
                }
            }
            tokens[token_iter][token_char_iter] = input[i];
            tc++;
            token_iter++;
        } else {
            tokens[token_iter][token_char_iter] = input[i];
            token_char_iter++;
            if (i < input_chars) {
                if (input[i + 1] == ' ' || i == input_chars - 1 || strchr(meta, input[i + 1])) {
                    tc++;
                }
            }
        }
    }
    return tc;
}
