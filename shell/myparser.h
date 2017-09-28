#ifndef MYPARSER_H_
#define MYPARSER_H_

#define MAX_BUFF_SIZE 512
#define MAX_TOKEN_SIZE 32
#define MAX_TOKENS 512

int fetch_input(char input[MAX_BUFF_SIZE], int prompt);

int get_tokens(int  input_chars,
               char input[MAX_BUFF_SIZE],
               char tokens[MAX_TOKENS][MAX_TOKEN_SIZE]);

void desc_tokens(char tokens[MAX_TOKENS][MAX_TOKEN_SIZE],
                 int  num_tokens);

#endif // MYPARSER_H_
