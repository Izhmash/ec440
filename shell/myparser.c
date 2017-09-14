#include <stdio.h>

#define MAX_BUFF_SIZE 512

void fetch_input(char *input);

int main()
{
    char user_input[MAX_BUFF_SIZE];

    fetch_input(user_input);
    
    //printf("%s\n", user_input);

    return 0;
}

void fetch_input(char *input)
{
    printf("%s", "my_parser> ");
    fgets(input, MAX_BUFF_SIZE, stdin);
}
