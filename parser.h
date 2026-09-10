#include <stdint.h>
#include <stdbool.h>
#include "lexer.h"

int8_t convertInto8bit(const char *source) {
    char *endptr;
    long val = strtol(source, &endptr, 10);
    if(val < INT8_MIN || val > INT8_MAX) return 1;

    int8_t result = (int8_t)val;
    return result;
}

// ==========================================
//               Parser Builder
// ==========================================

typedef enum ASTNodeType {
    // Tree
    AST_ROOT, AST_NODE, AST_PARENT, AST_CHILDREN,

    // rules
    AST_VAR, AST_FUNC, AST_RETURN, AST_STRUCT,

    // expression
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

ASTNode *ast_root() {
    ASTNode *root = (ASTNode*)malloc(sizeof(ASTNode));
    if(root == NULL) return NULL;

    root->type = AST_ROOT;
    root->value = NULL;
    root->op = TYPE_UNKNOWN;
    root->left = NULL;
    root->right = NULL;
    root->parent = NULL;
    root->children = NULL;
    root->node_count = 0;

    return root;
}

ASTNode *ast_node(ASTNode *parent) {
    ASTNode *node = ast_root();
    node->type = AST_NODE;
    node->parent = parent;

    ASTNode **new_children = (ASTNode**)realloc(
        parent->children,
        sizeof(ASTNode*) * (parent->node_count + 1)
    );

    if(new_children == NULL) {
        free(node);
        return NULL;
    }

    node->children = new_children;
    parent->children[parent->node_count] = node;

    parent->node_count++;
    return node;
}

ASTNode *ast_parent(ASTNode *parent) {
    ASTNode *node_parent = ast_node(parent);
    node_parent->type = AST_PARENT;
    return node_parent;
}

ASTNode *ast_children(ASTNode *parent) {
    ASTNode *children = ast_node(parent);
    children->type = AST_CHILDREN;
    return children;
}

void free_ast(ASTNode *buffer) {
    if(buffer == NULL) return;
    for(size_t i = 0; i < buffer->node_count; i++) {
        free_ast(buffer->children[i]);
    }

    free(buffer->value);
    free(buffer->children);
    free(buffer);
}

Token *current_t(Parser *p) {
    return &p->tokens[p->current];
}

void consume(Parser *p, TokenType type) {
    if(current_t(p)->type != type) return;
    p->current++;
}

int match(Parser *p, TokenType type) {
    if(current_t(p)->type == type) {
        p->current++;
        return 1;
    }
    return 0;
}

// ==========================================
//                SymbolTable
// ==========================================

typedef struct SymbolNode {
    char *label;
    uint8_t value;
    struct SymbolNode *next;
} SymbolNode;

typedef struct SymbolTable {
    SymbolNode *head;
} SymbolTable;

SymbolTable *add_table() {
    SymbolTable *table = (SymbolTable*)malloc(sizeof(SymbolTable));
    table->head = NULL;
    return table;
}

void add_variable(SymbolTable *table, const char *label, int8_t value) {
    SymbolNode *node = (SymbolNode*)malloc(sizeof(SymbolNode));
    if(node == NULL) return;

    node->label = strdup(label);
    node->value = value;
    node->next = table->head;
    table->head = node;
}

SymbolNode *lookup(SymbolTable *table, const char *label) {
    SymbolNode *current = table->head;
    while(current != NULL) {
        if(strcmp(current->label, label) == 0) return current;
        current = current->next;
    }
    return NULL;
}

bool update_var(SymbolTable *table, const char *label, int8_t value) {
    SymbolNode *symbol = lookup(table, label);
    if(symbol == NULL) return false;
    symbol->value = value;
    return true;
}

// ==========================================
//                Expression
// ==========================================

typedef struct StateLevel {
    TokenType type;
    int level;
    char *keyword;
} StateLevel;

const StateLevel levels[] = {
    {TYPE_BITWISE_OR, 1, "|"}, {TYPE_BITWISE_XOR, 2, "^"}, {TYPE_BITWISE_AND, 3, "&"},
    {TYPE_LSHIFT, 4, "<<"}, {TYPE_RSHIFT, 4, ">>"}, {TYPE_PLUS, 5, "+"},
    {TYPE_MIN, 5, "-"}, {TYPE_MUL, 6, "*"}, {TYPE_DIV, 6, "/"}, {TYPE_UNARY, 7, "~"}
};

int getLevel(TokenType type) {
    for (size_t i = 0; i < sizeof(levels) / sizeof(levels[0]); i++) {
        if (levels[i].type == type)
            return levels[i].level;
    }
    return 0;
}

ASTNode *create_binary_node(Token *tokens, ASTNode *left, ASTNode *right) {
    ASTNode *node = ast_root();
    node->type = AST_BINARY;
    node->value = strdup(tokens->value);

    node->left = left;
    node->right = right;
    return node;
}

ASTNode *create_unary_node(Token *tokens, ASTNode *operand) {
    ASTNode *node = ast_root();
    node->type = AST_UNARY;
    node->value = strdup(tokens->value);
    
    node->left = operand;
    return node;
}

ASTNode *create_value_node(Token *tokens) {
    ASTNode *node = ast_root();
    node->type = AST_LITERAL;
    node->value = strdup(tokens->value);
    return node;
}

ASTNode *parse_primary(Parser *p) {
    Token *tokens = current_t(p);
    if(tokens->type == TYPE_UNARY) {
        p->current++;
        ASTNode *operand = parse_primary(p);

        if(operand == NULL) return NULL;
        return create_unary_node(tokens, operand);
    }

    if(tokens->type == TYPE_INT_LITERAL ||
        tokens->type == TYPE_HEX_LITERAL ||
        tokens->type == TYPE_ID) {
            p->current++;
            return create_value_node(tokens);
        }
    
    return NULL;
}

ASTNode *parse_expression(Parser *p, int min_level) {
    ASTNode *left = parse_primary(p);

    while(1) {
        Token *op = current_t(p);
        int level = getLevel(op->type);
        if(level < min_level) break;

        p->current++;
        ASTNode *right = parse_expression(p, level + 1);
        if(right == NULL) return NULL;

        left = create_binary_node(op, left, right);
    }
    return left;
}

int8_t evaluate(ASTNode *node) {
    if(node == NULL) return 0;
    if(node->type == AST_LITERAL) return convertInto8bit(node->value);

    int8_t left = evaluate(node->left);
    int8_t right = evaluate(node->right);

    switch(node->op) {
        case TYPE_PLUS: return left + right;
        case TYPE_MIN: return left - right;
        case TYPE_MUL: return left * right;
        case TYPE_DIV:
            if(right == 0) return 0;
            return left / right;
        case TYPE_BITWISE_AND: return left & right;
        case TYPE_BITWISE_OR: return left | right;
        case TYPE_BITWISE_XOR: return left ^ right;
        case TYPE_UNARY: return ~left;
        case TYPE_LSHIFT: return left << right;
        case TYPE_RSHIFT: return left >> right;
        default: return 0;
    }
}

// ==========================================
//                   Struct
// ==========================================

typedef struct Symbol {
    char *name;
    int8_t size;
    int8_t offset;

    struct Symbol *parent;
    struct Symbol *children;
    struct Symbol *next;
} Symbol;

Symbol *root_symbol() {
    Symbol *sym = (Symbol *)malloc(sizeof(Symbol));
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
    symbol->name = name;
    symbol->size = size;
    symbol->offset = offset;
    return symbol;
}

void add_symbol_child(Symbol *parent, Symbol *children) {
    children->parent = parent;
    if(parent->children == NULL) {
        parent->children = children;
        return;
    }

    Symbol *c = parent->children;
    while(c->next != NULL) c = c->next;
    c->next = children;
}