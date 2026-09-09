#include "parser.h"
#include "ast.h"
#include <stdio.h>

char *read(const char *source) {
    FILE *fptr = fopen(source, "r");
    if(!fptr) return NULL;

    fseek(fptr, 0, SEEK_END);
    long length = ftell(fptr);
    rewind(fptr);

    char *buffer = (char*)malloc(length + 1);
    if(!buffer) {
        fclose(fptr);
        return NULL;
    }

    size_t reader = fread(buffer, 1, length, fptr);
    buffer[reader] = '\0';

    fclose(fptr);
    return buffer;
}

int main() {
    char *buffer = read("main.txt");
    
    Lexer lexer = {
        .source = buffer,
        .cursor = 0
    };

    Token *token = get_next_token(&lexer);

    Parser parser = {
        .tokens = token,
        .current = 0,
        .count = 0
    };

    ASTNode *parse = parse_access(&parser);

    printf("\nACCESS:\n");
    printf("Root: %s\n", parse->value);

    for (size_t i = 0; i < parse->node_count; i++) {
        printf("Member: %s\n",
               parse->children[i]->value);
    }

    free_ast(parse);
    free(token);
    free(buffer);
}