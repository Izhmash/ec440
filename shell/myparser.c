#include <stdio.h>

#define MAX_BUFF_SIZE 512
#define MAX_TOKEN_SIZE 32
#define MAX_TOKENS 512

void fetch_input(char input[MAX_BUFF_SIZE]);
void get_tokens(char input[MAX_BUFF_SIZE], char tokens[MAX_TOKENS][MAX_TOKEN_SIZE]);

int main()
{
    char user_input[MAX_BUFF_SIZE];
    char tokens[MAX_TOKENS][MAX_TOKEN_SIZE];

    fetch_input(user_input);
    get_tokens(user_input, tokens);

    for(int i = 0; i < MAX_TOKENS; ++i) {
        printf("%s", tokens[i]);
    }
    
    return 0;
}

void fetch_input(char input[MAX_BUFF_SIZE])
{
    printf("%s", "my_parser> ");
    fgets(input, MAX_BUFF_SIZE, stdin);
}

void get_tokens(char input[MAX_BUFF_SIZE], char tokens[MAX_TOKENS][MAX_TOKEN_SIZE])
{
}
