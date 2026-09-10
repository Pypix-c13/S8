#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>

static char *dup_string(const char *source) {
    if (!source) return NULL;

    size_t length = strlen(source);
    char *copy = (char*)malloc(length + 1);
    if (!copy) return NULL;

    memcpy(copy, source, length + 1);
    return copy;
}

typedef enum TokenType {
    TYPE_INT, TYPE_FUNCTION, TYPE_RETURN, TYPE_STRUCT,
    TYPE_LBRACE, TYPE_RBRACE, TYPE_LPAREN, TYPE_RPAREN,
    TYPE_COMMA, TYPE_SEMICOLON, TYPE_DOT, TYPE_EQUAL,
    TYPE_PLUS, TYPE_MIN, TYPE_MUL, TYPE_DIV, TYPE_UNARY,
    TYPE_BITWISE_AND, TYPE_BITWISE_OR, TYPE_BITWISE_XOR,
    TYPE_LSHIFT, TYPE_RSHIFT,
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

Token *token_init(void) {
    Token *tokens = (Token*)malloc(sizeof(Token));
    if (!tokens) return NULL;

    tokens->type = TYPE_UNKNOWN;
    tokens->value = NULL;
    return tokens;
}

Token *addToken(TokenType type, const char *value) {
    Token *tokens = token_init();
    if (!tokens) return NULL;

    tokens->type = type;
    tokens->value = dup_string(value);

    if (!tokens->value) {
        free(tokens);
        return NULL;
    }

    return tokens;
}

const Vector keylist[] = {
    {"int", TYPE_INT}, {"func", TYPE_FUNCTION}, {"return", TYPE_RETURN},
    {"struct", TYPE_STRUCT}, {"{", TYPE_LBRACE}, {"}", TYPE_RBRACE},
    {"(", TYPE_LPAREN}, {")", TYPE_RPAREN}, {",", TYPE_COMMA},
    {";", TYPE_SEMICOLON}, {".", TYPE_DOT}, {"=", TYPE_EQUAL},
    {"+", TYPE_PLUS}, {"-", TYPE_MIN}, {"*", TYPE_MUL},
    {"/", TYPE_DIV}, {"~", TYPE_UNARY}, {"&", TYPE_BITWISE_AND},
    {"|", TYPE_BITWISE_OR}, {"^", TYPE_BITWISE_XOR},
    {"<<", TYPE_LSHIFT}, {">>", TYPE_RSHIFT},
    {NULL, TYPE_UNKNOWN}
};

Token *isNumber(Lexer *lexer, Token *tokens) {
    size_t start = lexer->cursor;

    while (isdigit((unsigned char)lexer->source[lexer->cursor]))
        lexer->cursor++;

    size_t length = lexer->cursor - start;

    tokens->type = TYPE_INT_LITERAL;
    tokens->value = (char*)malloc(length + 1);

    if (!tokens->value) {
        free(tokens);
        return NULL;
    }

    memcpy(tokens->value, &lexer->source[start], length);
    tokens->value[length] = '\0';

    return tokens;
}

Token *isHex(Lexer *lexer, Token *tokens) {
    size_t start = lexer->cursor;
    lexer->cursor += 2;

    while (isxdigit((unsigned char)lexer->source[lexer->cursor]))
        lexer->cursor++;

    size_t length = lexer->cursor - start;

    tokens->type = TYPE_HEX_LITERAL;
    tokens->value = (char*)malloc(length + 1);

    if (!tokens->value) {
        free(tokens);
        return NULL;
    }

    memcpy(tokens->value, &lexer->source[start], length);
    tokens->value[length] = '\0';

    return tokens;
}

Token *isID(Lexer *lexer, Token *tokens) {
    size_t start = lexer->cursor;

    while (isalnum((unsigned char)lexer->source[lexer->cursor]) ||
           lexer->source[lexer->cursor] == '_')
        lexer->cursor++;

    size_t length = lexer->cursor - start;

    tokens->value = (char*)malloc(length + 1);

    if (!tokens->value) {
        free(tokens);
        return NULL;
    }

    memcpy(tokens->value, &lexer->source[start], length);
    tokens->value[length] = '\0';
    tokens->type = TYPE_ID;

    for (size_t i = 0; keylist[i].keyword != NULL; i++) {
        if (strcmp(tokens->value, keylist[i].keyword) == 0) {
            tokens->type = keylist[i].type;
            break;
        }
    }

    return tokens;
}

void isIgnore(Lexer *lexer) {
    while (lexer->source[lexer->cursor] != '\0') {
        if (isspace((unsigned char)lexer->source[lexer->cursor])) {
            lexer->cursor++;
            continue;
        }

        if (lexer->source[lexer->cursor] == '/' &&
            lexer->source[lexer->cursor + 1] == '/') {
            while (lexer->source[lexer->cursor] != '\n' &&
                   lexer->source[lexer->cursor] != '\0')
                lexer->cursor++;

            continue;
        }

        break;
    }
}

Token *get_next_token(Lexer *lexer) {
    Token *tokens = token_init();
    if (!tokens) return NULL;

    isIgnore(lexer);

    if (lexer->source[lexer->cursor] == '\0') {
        tokens->type = TYPE_EOF;
        tokens->value = dup_string("EOF");
        return tokens;
    }

    if (lexer->source[lexer->cursor] == '0' &&
        (lexer->source[lexer->cursor + 1] == 'x' ||
         lexer->source[lexer->cursor + 1] == 'X'))
        return isHex(lexer, tokens);

    if (isdigit((unsigned char)lexer->source[lexer->cursor]))
        return isNumber(lexer, tokens);

    if (isalpha((unsigned char)lexer->source[lexer->cursor]) ||
        lexer->source[lexer->cursor] == '_')
        return isID(lexer, tokens);

    for (size_t i = 0; keylist[i].keyword != NULL; i++) {
        size_t length = strlen(keylist[i].keyword);

        if (strncmp(&lexer->source[lexer->cursor],
                    keylist[i].keyword, length) == 0) {
            lexer->cursor += length;
            tokens->type = keylist[i].type;
            tokens->value = (char*)malloc(length + 1);

            if (!tokens->value) {
                free(tokens);
                return NULL;
            }

            memcpy(tokens->value, keylist[i].keyword, length);
            tokens->value[length] = '\0';

            return tokens;
        }
    }

    tokens->type = TYPE_UNKNOWN;
    tokens->value = (char*)malloc(2);

    if (tokens->value) {
        tokens->value[0] = lexer->source[lexer->cursor];
        tokens->value[1] = '\0';
    }

    lexer->cursor++;
    return tokens;
}

int8_t convertInto8bit(const char *source) {
    if (!source) return 0;

    char *endptr;
    int base = 10;

    if (source[0] == '0' &&
        (source[1] == 'x' || source[1] == 'X'))
        base = 16;

    long val = strtol(source, &endptr, base);
    if (*endptr != '\0') return 0;
    if (val < INT8_MIN || val > INT8_MAX) return 0;

    return (int8_t)val;
}

/* Parser Builder */

typedef enum ASTNodeType {
    AST_ROOT, AST_NODE, AST_PARENT, AST_CHILDREN,
    AST_VAR, AST_FUNC, AST_RETURN, AST_STRUCT,
    AST_LITERAL, AST_BINARY, AST_UNARY, AST_EXPRESSION
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    char *value;
    TokenType op;

    struct ASTNode *next;
    struct ASTNode *left;
    struct ASTNode *right;

    struct ASTNode *parent;
    struct ASTNode **children;
    size_t node_count;
} ASTNode;

typedef struct Parser {
    Token *tokens;
    size_t current;
    size_t count;
} Parser;

ASTNode *ast_root(void) {
    ASTNode *root = (ASTNode*)malloc(sizeof(ASTNode));
    if (!root) return NULL;

    root->type = AST_ROOT;
    root->value = NULL;
    root->op = TYPE_UNKNOWN;
    root->next = NULL;
    root->left = NULL;
    root->right = NULL;
    root->parent = NULL;
    root->children = NULL;
    root->node_count = 0;

    return root;
}

ASTNode *ast_node(ASTNode *parent) {
    ASTNode *node = ast_root();
    if (!node) return NULL;

    node->type = AST_NODE;
    node->parent = parent;

    if (parent) {
        ASTNode **new_children = (ASTNode**)realloc(
            parent->children,
            sizeof(ASTNode*) * (parent->node_count + 1)
        );

        if (!new_children) {
            free(node);
            return NULL;
        }

        parent->children = new_children;
        parent->children[parent->node_count] = node;
        parent->node_count++;
    }

    return node;
}

ASTNode *ast_parent(ASTNode *parent) {
    ASTNode *node_parent = ast_node(parent);
    if (node_parent) node_parent->type = AST_PARENT;
    return node_parent;
}

ASTNode *ast_children(ASTNode *parent) {
    ASTNode *children = ast_node(parent);
    if (children) children->type = AST_CHILDREN;
    return children;
}

void free_ast(ASTNode *buffer) {
    if (!buffer) return;
    for (size_t i = 0; i < buffer->node_count; i++) free_ast(buffer->children[i]);

    if (buffer->left) free_ast(buffer->left);
    if (buffer->right) free_ast(buffer->right);

    free(buffer->value);
    free(buffer->children);
    free(buffer);
}

Token *current_t(Parser *p) {
    return &p->tokens[p->current];
}

void consume(Parser *p, TokenType type) {
    if (current_t(p)->type == type) p->current++;
}

int match(Parser *p, TokenType type) {
    if (current_t(p)->type == type) {
        p->current++;
        return 1;
    }

    return 0;
}

/* SymbolTable */

typedef struct SymbolNode {
    char *label;
    uint8_t value;
    struct SymbolNode *next;
} SymbolNode;

typedef struct SymbolTable {
    SymbolNode *head;
} SymbolTable;

SymbolTable *add_table(void) {
    SymbolTable *table = (SymbolTable*)malloc(sizeof(SymbolTable));
    if (table) table->head = NULL;
    return table;
}

void add_variable(SymbolTable *table, const char *label, int8_t value) {
    if (!table) return;
    SymbolNode *node = (SymbolNode*)malloc(sizeof(SymbolNode));
    if (!node) return;

    node->label = dup_string(label);
    node->value = value;
    node->next = table->head;
    table->head = node;
}

SymbolNode *lookup(SymbolTable *table, const char *label) {
    if (!table) return NULL;
    SymbolNode *current = table->head;

    while (current != NULL) {
        if (strcmp(current->label, label) == 0) return current;
        current = current->next;
    }

    return NULL;
}

bool update_var(SymbolTable *table, const char *label, int8_t value) {
    SymbolNode *symbol = lookup(table, label);
    if (symbol == NULL) return false;

    symbol->value = value;
    return true;
}

/* Expression */

typedef struct StateLevel {
    TokenType type;
    int level;
    char *keyword;
} StateLevel;

const StateLevel levels[] = {
    {TYPE_BITWISE_OR, 1, "|"},
    {TYPE_BITWISE_XOR, 2, "^"},
    {TYPE_BITWISE_AND, 3, "&"},
    {TYPE_LSHIFT, 4, "<<"},
    {TYPE_RSHIFT, 4, ">>"},
    {TYPE_PLUS, 5, "+"},
    {TYPE_MIN, 5, "-"},
    {TYPE_MUL, 6, "*"},
    {TYPE_DIV, 6, "/"},
    {TYPE_UNARY, 7, "~"}
};

int getLevel(TokenType type) {
    for (size_t i = 0; i < sizeof(levels) / sizeof(levels[0]); i++) {
        if (levels[i].type == type) return levels[i].level;
    }
    return 0;
}

ASTNode *create_binary_node(Token *tokens, ASTNode *left, ASTNode *right) {
    ASTNode *node = ast_root();
    if (!node) return NULL;

    node->type = AST_BINARY;
    node->value = dup_string(tokens->value);
    node->op = tokens->type;
    node->left = left;
    node->right = right;

    return node;
}

ASTNode *create_unary_node(Token *tokens, ASTNode *operand) {
    ASTNode *node = ast_root();
    if (!node) return NULL;

    node->type = AST_UNARY;
    node->value = dup_string(tokens->value);
    node->op = tokens->type;
    node->left = operand;

    return node;
}

ASTNode *create_value_node(Token *tokens) {
    ASTNode *node = ast_root();
    if (!node) return NULL;

    node->type = AST_LITERAL;
    node->value = dup_string(tokens->value);

    return node;
}

ASTNode *parse_primary(Parser *p) {
    Token *tokens = current_t(p);

    if (tokens->type == TYPE_UNARY) {
        p->current++;

        ASTNode *operand = parse_primary(p);
        if (operand == NULL) return NULL;

        return create_unary_node(tokens, operand);
    }

    if (tokens->type == TYPE_INT_LITERAL ||
        tokens->type == TYPE_HEX_LITERAL) {
        p->current++;
        return create_value_node(tokens);
    }

    if(tokens->type == TYPE_ID) {
        p->current++;
        ASTNode *node = create_value_node(tokens);
        if (!node) return NULL;

        if (match(p, TYPE_DOT)) {
            Token *member = current_t(p);
            if (member->type != TYPE_ID) {
                free_ast(node);
                return NULL;
            }
            p->current++;
            return node;
        }
        return node;
    }

    return NULL;
}

ASTNode *parse_expression(Parser *p, int min_level) {
    ASTNode *left = parse_primary(p);
    if (!left) return NULL;

    while (1) {
        Token *op = current_t(p);
        int level = getLevel(op->type);

        if (level < min_level) break;
        p->current++;

        ASTNode *right = parse_expression(p, level + 1);
        if (right == NULL) return NULL;

        left = create_binary_node(op, left, right);
        if (!left) return NULL;
    }

    return left;
}

int8_t evaluate(ASTNode *node) {
    if (!node) return 0;
    if (node->type == AST_LITERAL) return convertInto8bit(node->value);
    if (node->op == TYPE_UNARY) return ~evaluate(node->left);

    int8_t left = evaluate(node->left);
    int8_t right = evaluate(node->right);

    switch (node->op) {
        case TYPE_PLUS: return left + right;
        case TYPE_MIN: return left - right;
        case TYPE_MUL: return left * right;
        case TYPE_DIV: return (right == 0) ? 0 : left / right;
        case TYPE_BITWISE_AND: return left & right;
        case TYPE_BITWISE_OR: return left | right;
        case TYPE_BITWISE_XOR: return left ^ right;
        case TYPE_LSHIFT: return left << right;
        case TYPE_RSHIFT: return left >> right;
        default: return 0;
    }
}

/* Struct */

typedef struct Symbol {
    char *name;
    int8_t size;
    int8_t offset;

    struct Symbol *parent;
    struct Symbol *children;
    struct Symbol *next;
} Symbol;

Symbol *root_symbol(void) {
    Symbol *sym = (Symbol*)malloc(sizeof(Symbol));
    if (!sym) return NULL;

    sym->name = NULL;
    sym->size = 0;
    sym->offset = 0;
    sym->parent = NULL;
    sym->children = NULL;
    sym->next = NULL;

    return sym;
}

Symbol *new_symbol(char *name, int8_t size, int8_t offset) {
    Symbol *symbol = root_symbol();
    if (!symbol) return NULL;

    symbol->name = dup_string(name);
    symbol->size = size;
    symbol->offset = offset;

    return symbol;
}

void add_symbol_child(Symbol *parent, Symbol *children) {
    if (!parent || !children) return;
    children->parent = parent;

    if (parent->children == NULL) {
        parent->children = children;
        return;
    }

    Symbol *c = parent->children;
    while (c->next != NULL) c = c->next;
    c->next = children;
}

/* Function */

typedef struct Parameter {
    char *name;
    int8_t value;
    struct Parameter *next;
} Parameter;

typedef struct Function {
    char *name;
    Parameter *param;
    size_t param_count;
    struct Function *next;
} Function;

Parameter *new_param(char *name) {
    Parameter *param = (Parameter*)malloc(sizeof(Parameter));
    if (!param) return NULL;

    param->name = dup_string(name);
    param->value = 0;
    param->next = NULL;

    return param;
}

Function *add_function(char *name) {
    Function *func = (Function*)malloc(sizeof(Function));
    if (!func) return NULL;

    func->name = dup_string(name);
    func->param = NULL;
    func->param_count = 0;
    func->next = NULL;

    return func;
}

void add_parameter(Function *function, Parameter *param) {
    if (!function || !param) return;
    if (function->param == NULL) {
        function->param = param;
        function->param_count++;
        return;
    }

    Parameter *current = function->param;
    while (current->next != NULL) current = current->next;

    current->next = param;
    function->param_count++;
}

void bind_arguments(Function *function, int8_t *arguments, size_t count) {
    if (!function) return;
    Parameter *current = function->param;

    for (size_t i = 0; i < count && current != NULL; i++) {
        current->value = arguments[i];
        current = current->next;
    }
}

Parameter *lookup_parameter(Function *function, char *name) {
    if (!function || !name) return NULL;
    Parameter *current = function->param;

    while (current != NULL) {
        if (strcmp(current->name, name) == 0) return current;
        current = current->next;
    }

    return NULL;
}

ASTNode *parse_int(Parser *p) {
    consume(p, TYPE_INT);

    Token *id = current_t(p);
    if (id->type != TYPE_ID) return NULL;

    p->current++;

    ASTNode *node = ast_root();
    if (!node) return NULL;

    node->type = AST_VAR;
    node->value = dup_string(id->value);

    if (current_t(p)->type == TYPE_SEMICOLON) {
        p->current++;
        return node;
    }

    if (current_t(p)->type == TYPE_EQUAL) {
        p->current++;

        ASTNode *expr = parse_expression(p, 1);
        if (!expr) {
            free_ast(node);
            return NULL;
        }

        node->left = expr;
        consume(p, TYPE_SEMICOLON);

        return node;
    }

    free_ast(node);
    return NULL;
}

ASTNode *parse_return(Parser *p) {
    consume(p, TYPE_RETURN);
    ASTNode *node = ast_root();

    if (!node) return NULL;
    node->type = AST_RETURN;

    ASTNode *expr = parse_expression(p, 1);
    if (!expr) {
        free_ast(node);
        return NULL;
    }

    node->left = expr;
    consume(p, TYPE_SEMICOLON);

    return node;
}

ASTNode *parse_struct(Parser *p) {
    consume(p, TYPE_STRUCT);
    Token *id = current_t(p);

    if (id->type != TYPE_ID) return NULL;
    p->current++;

    ASTNode *node = ast_root();
    if (!node) return NULL;

    node->type = AST_STRUCT;
    node->value = dup_string(id->value);

    if (!match(p, TYPE_LBRACE)) {
        free_ast(node);
        return NULL;
    }

    while (current_t(p)->type != TYPE_RBRACE &&
           current_t(p)->type != TYPE_EOF) {
        if (current_t(p)->type == TYPE_INT) {
            ASTNode *member = parse_int(p);
            if (!member) {
                free_ast(node);
                return NULL;
            }

            member->parent = node;
            ASTNode **new_children = (ASTNode**)realloc(
                node->children,
                sizeof(ASTNode*) * (node->node_count + 1)
            );

            if (!new_children) {
                free_ast(member);
                free_ast(node);
                return NULL;
            }

            node->children = new_children;
            node->children[node->node_count] = member;
            node->node_count++;

            continue;
        }

        free_ast(node);
        return NULL;
    }

    if (!match(p, TYPE_RBRACE)) {
        free_ast(node);
        return NULL;
    }

    return node;
}

Parameter *parse_param(Parser *p) {
    if (current_t(p)->type != TYPE_INT) return NULL;
    p->current++;

    Token *name = current_t(p);
    if (name->type != TYPE_ID) return NULL;

    p->current++;
    return new_param(name->value);
}

void parse_body(Parser *p) {
    while (current_t(p)->type != TYPE_RBRACE &&
           current_t(p)->type != TYPE_EOF) {
        switch (current_t(p)->type) {
            case TYPE_INT:
                parse_int(p);
                break;

            case TYPE_RETURN:
                parse_return(p);
                break;

            default:
                p->current++;
                break;
        }
    }
}

Function *parse_function(Parser *p) {
    if (!match(p, TYPE_FUNCTION)) return NULL;

    Token *name = current_t(p);
    if (name->type != TYPE_ID) return NULL;
    p->current++;

    Function *function = add_function(name->value);
    if (!function) return NULL;
    if (!match(p, TYPE_LPAREN)) return NULL;

    if (current_t(p)->type != TYPE_RPAREN) {
        while (true) {
            Parameter *param = parse_param(p);
            if (!param) return NULL;
            add_parameter(function, param);
            if (!match(p, TYPE_COMMA)) break;
        }
    }

    if (!match(p, TYPE_RPAREN)) return NULL;
    if (!match(p, TYPE_LBRACE)) return NULL;

    parse_body(p);

    if (!match(p, TYPE_RBRACE)) return NULL;
    return function;
}

void parse_argument(Parser *p) {
    parse_expression(p, 1);
    while (match(p, TYPE_COMMA)) parse_expression(p, 1);
}

void parse_function_call(Parser *p) {
    consume(p, TYPE_ID);
    consume(p, TYPE_LPAREN);

    parse_argument(p);

    consume(p, TYPE_RPAREN);
    consume(p, TYPE_SEMICOLON);
}

ASTNode *parse_program(Parser *p) {
    ASTNode *root = ast_root();
    if (!root) return NULL;

    while (current_t(p)->type != TYPE_EOF) {
        ASTNode *node = NULL;

        switch (current_t(p)->type) {
            case TYPE_INT:
                node = parse_int(p);
                break;

            case TYPE_STRUCT:
                node = parse_struct(p);
                break;

            case TYPE_RETURN:
                node = parse_return(p);
                break;

            case TYPE_FUNCTION:
                if (!parse_function(p)) {
                    free_ast(root);
                    return NULL;
                }
                break;

            case TYPE_ID:
                if (p->tokens[p->current + 1].type == TYPE_DOT) {
                    node = parse_expression(p, 1);

                    if (match(p, TYPE_EQUAL)) {
                        ASTNode *value = parse_expression(p, 1);
                        if (!value) {
                            free_ast(node);
                            free_ast(root);
                            return NULL;
                        }
                        node->left = value;
                    }
                    consume(p, TYPE_SEMICOLON);
                } else {
                    parse_function_call(p);
                }
                break;

            default:
                free_ast(root);
                return NULL;
        }

        if (node) {
            node->parent = root;
            ASTNode **new_children = (ASTNode**)realloc(
                root->children,
                sizeof(ASTNode*) * (root->node_count + 1)
            );

            if (!new_children) {
                free_ast(node);
                free_ast(root);
                return NULL;
            }

            root->children = new_children;
            root->children[root->node_count] = node;
            root->node_count++;
        }
    }

    return root;
}

/* File */

bool is_file(const char *source) {
    FILE *fptr = fopen(source, "r");
    if (!fptr) {
        printf("File '%s' not found!.\n", source);
        return false;
    }

    struct stat path_stat;
    if (stat(source, &path_stat) != 0) {
        fclose(fptr);
        return false;
    }

    if (!S_ISREG(path_stat.st_mode)) {
        printf("No such file!.\n");
        fclose(fptr);
        return false;
    }

    const char *dot = strrchr(source, '.');

    if (!dot || strcmp(dot, ".s8") != 0) {
        printf("Extension must be .s8\n");
        fclose(fptr);
        return false;
    }

    fclose(fptr);
    return true;
}

char *read(const char *source) {
    if (!is_file(source)) return NULL;

    FILE *fptr = fopen(source, "r");
    if (!fptr) return NULL;

    fseek(fptr, 0, SEEK_END);

    long length = ftell(fptr);
    rewind(fptr);

    char *buffer = (char*)malloc(length + 1);

    if (!buffer) {
        fclose(fptr);
        return NULL;
    }

    size_t reader = fread(buffer, 1, length, fptr);
    buffer[reader] = '\0';

    fclose(fptr);
    return buffer;
}

typedef struct CommandLine {
    const char *key;
    const char *des;
} CommandLine;

const CommandLine cmd[] = {
    {"help", "show help message"},
    {"version", "show newest version"}
};

const size_t cmd_count = sizeof(cmd) / sizeof(cmd[0]);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("usage: seight [file_or_options]\n");
        return 1;
    }

    char *c = argv[1];

    if (strcmp(c, "help") == 0) {
        printf("Options:\n");
        for (size_t i = 0; i < cmd_count; i++)
            printf("    %s - %s\n", cmd[i].key, cmd[i].des);
        return 0;
    }

    if (strcmp(c, "version") == 0) {
        printf("Version: 1.0\n");
        return 0;
    }

    char *buffer = read(c);
    if (!buffer) return 1;

    Lexer lexer = {
        .source = buffer,
        .cursor = 0
    };

    size_t token_count = 0;
    size_t capacity = 16;

    Token *tokens = (Token*)malloc(sizeof(Token) * capacity);

    if (!tokens) {
        free(buffer);
        return 1;
    }

    while (true) {
        Token *token = get_next_token(&lexer);
        if (!token) break;

        if (token_count >= capacity) {
            capacity *= 2;
            Token *new_tokens = (Token*)realloc(
                tokens,
                sizeof(Token) * capacity
            );

            if (!new_tokens) {
                free(token->value);
                free(token);
                for (size_t i = 0; i < token_count; i++) free(tokens[i].value);

                free(tokens);
                free(buffer);
                return 1;
            }

            tokens = new_tokens;
        }

        tokens[token_count] = *token;
        free(token);

        token_count++;
        if (tokens[token_count - 1].type == TYPE_EOF) break;
    }

    Parser p = {
        .tokens = tokens,
        .current = 0,
        .count = token_count
    };

    ASTNode *root = parse_program(&p);

    if (!root) {
        printf("Parse error\n");
        for (size_t i = 0; i < token_count; i++) free(tokens[i].value);

        free(tokens);
        free(buffer);
        return 1;
    }

    free_ast(root);
    for (size_t i = 0; i < token_count; i++) free(tokens[i].value);

    free(tokens);
    free(buffer);
    return 0;
}