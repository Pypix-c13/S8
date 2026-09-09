#include <ctype.h>
#include <string.h>
#include <stdlib.h>

typedef enum TokenType {
    // Syntax
    TYPE_INT, TYPE_FUNCTION, TYPE_RETURN, TYPE_STRUCT,

    // Symbols
    TYPE_LBRACE, TYPE_RBRACE, TYPE_LPAREN, TYPE_RPAREN,
    TYPE_COMMA, TYPE_SEMICOLON, TYPE_DOT, TYPE_EQUAL,

    // Operators
    TYPE_PLUS, TYPE_MIN, TYPE_MUL, TYPE_DIV, TYPE_UNARY,
    TYPE_BITWISE_AND, TYPE_BITWISE_OR, TYPE_BITWISE_XOR,
    TYPE_LSHIFT, TYPE_RSHIFT,

    // Internal Lexer
    TYPE_ID, TYPE_INT_LITERAL, TYPE_HEX_LITERAL, TYPE_EOF,
    TYPE_UNKNOWN
} TokenType;

typedef struct Token {
    TokenType type;
    char *value;
} Token;

typedef struct Lexer {
    const char *source;
    size_t cursor;
} Lexer;

typedef struct Vector {
    const char *keyword;
    TokenType type;
} Vector;

Token *token_init() {
    Token *tokens = (Token*)malloc(sizeof(Token));
    tokens->type = TYPE_UNKNOWN;
    tokens->value = NULL;
    return tokens;
}

Token *addToken(TokenType type, const char *value) {
    Token *tokens = token_init();
    if(tokens == NULL) return;

    tokens->type = type;
    tokens->value = (char*)malloc(strlen(value) + 1);

    if(tokens->value == NULL) {
        free(tokens);
        return NULL;
    }

    strcpy(tokens->value, value);
    return tokens;
}

const Vector keylist[] = {
    {"int", TYPE_INT}, {"func", TYPE_FUNCTION}, {"return", TYPE_RETURN},
    {"struct", TYPE_STRUCT}, {"{", TYPE_LBRACE}, {"}", TYPE_RBRACE},
    {"(", TYPE_LPAREN}, {")", TYPE_RPAREN}, {",", TYPE_COMMA},
    {";", TYPE_SEMICOLON}, {".", TYPE_DOT}, {"=", TYPE_EQUAL},
    {"+", TYPE_PLUS}, {"-", TYPE_MIN}, {"*", TYPE_MUL},
    {"/", TYPE_DIV}, {"~", TYPE_UNARY}, {"&", TYPE_BITWISE_AND},
    {"|", TYPE_BITWISE_OR}, {"^", TYPE_BITWISE_XOR}, {"<<", TYPE_LSHIFT},
    {">>", TYPE_RSHIFT}, {NULL, TYPE_UNKNOWN}
};

Token *isNumber(Lexer *lexer, Token *tokens) {
    size_t start = lexer->cursor;
    while(isdigit(lexer->source[lexer->cursor])) {
        lexer->cursor++;
    }

    size_t length = lexer->cursor - start;
    tokens->type = TYPE_INT_LITERAL;
    tokens->value = (char*)malloc(length + 1);

    memcpy(tokens->value, &lexer->source[start], length);
    tokens->value[length] = '\0';
    return tokens;
}

Token *isHex(Lexer *lexer, Token *tokens) {
    size_t start = lexer->cursor;
    lexer->cursor += 2;

    while(isxdigit(lexer->source[lexer->cursor])) {
        lexer->cursor++;
    }

    size_t length = lexer->cursor - start;
    tokens->type = TYPE_HEX_LITERAL;
    tokens->value = (char*)malloc(length + 1);

    memcpy(tokens->value, &lexer->source[start], length);
    tokens->value[length] = '\0';
    return tokens;
}

Token *isID(Lexer *lexer, Token *tokens) {
    size_t start = lexer->cursor;
    while(isalnum(lexer->source[lexer->cursor]) || lexer->source[lexer->cursor] == '_') {
        lexer->cursor++;
    }

    size_t length = lexer->cursor - start;
    tokens->type = TYPE_ID;
    tokens->value = (char*)malloc(length + 1);

    memcpy(tokens->value, &lexer->source[start], length);
    tokens->value[length] = '\0';
    return tokens;
}

void isIgnore(Lexer *lexer) {
    while(lexer->source[lexer->cursor] != '\0') {
        if(isspace(lexer->source[lexer->cursor])) {
            lexer->cursor++;
            continue;
        }

        if(lexer->source[lexer->cursor] == '/' &&
            lexer->source[lexer->cursor + 1] == '/') {
                while (lexer->source[lexer->cursor] != '\n' &&
                    lexer->source[lexer->cursor] != '\0') {
                    lexer->cursor++;
                }
                continue;
            }
        break;
    }
}

Token *get_next_token(Lexer *lexer) {
    Token *tokens = token_init();
    while(lexer->source[lexer->cursor] != '\0') {
        isIgnore(lexer);
        if(isdigit(lexer->source[lexer->cursor])) {
            return isNumber(lexer, tokens);
        }
        if(isxdigit(lexer->source[lexer->cursor]) || lexer->source[lexer->cursor] == '0' && (lexer->source[lexer->cursor + 1] == 'x' || lexer->source[lexer->cursor + 1] == 'X')) {
            return isHex(lexer, tokens);
        }
        if(isalpha(lexer->source[lexer->cursor]) || lexer->source[lexer->cursor] == '_') {
            return isID(lexer, tokens);
        }

        for(size_t i = 0;keylist[i].keyword != NULL;i++) {
            if(strcmp(lexer->source, keylist[i].keyword) == 0) {
                return addToken(keylist[i].type, keylist[i].keyword);
            }
        }
    }

    tokens->type = TYPE_EOF;
    tokens->value = NULL;
    return tokens;
}