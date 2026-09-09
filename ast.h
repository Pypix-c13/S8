#include "parser.h"

ASTNode *parse_int(Parser *p) {
    match(p, TYPE_INT);
    consume(p, TYPE_INT);

    Token *id = current_t(p);
    if(id->type != TYPE_ID) return NULL;
    p->current++;

    ASTNode *node = ast_node((ASTNode*)id->value);

    if(current_t(p)->type == TYPE_SEMICOLON) {
        node->value = (char*)convertInto8bit("0");
        p->current++;
        return node;
    }

    if(current_t(p)->type == TYPE_EQUAL) {
        p->current++;

        ASTNode *parent = parse_expression(p, 1);
        if(parent == NULL) return NULL;
        ast_children(parent);

        match(p, TYPE_SEMICOLON);
        consume(p, TYPE_SEMICOLON);
        return node;
    }

    return NULL;
}

ASTNode *parse_return(Parser *p) {
    match(p, TYPE_RETURN);
    consume(p, TYPE_RETURN);

    ASTNode *node = parse_expression(p, 1);
    if(node == NULL) return NULL;
    ast_children(node);

    match(p, TYPE_SEMICOLON);
    consume(p, TYPE_SEMICOLON);
    return node;
}

ASTNode *parse_struct(Parser *p) {
    match(p, TYPE_STRUCT);
    consume(p, TYPE_STRUCT);

    Token *id = current_t(p);
    if(id->type != TYPE_ID) return NULL;
    p->current++;

    ASTNode *node = ast_node((ASTNode*)id->value);

    match(p, TYPE_LBRACE);
    consume(p, TYPE_LBRACE);

    while (current_t(p)->type != TYPE_RBRACE) {
        if (current_t(p)->type == TYPE_INT) {
            ASTNode *member = parse_int(p);
            if (member == NULL) return NULL;
            ast_children(member);
            continue;
        }

        return NULL;
    }

    match(p, TYPE_RBRACE);
    consume(p, TYPE_RBRACE);
    return node;
}