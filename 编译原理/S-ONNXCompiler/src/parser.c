#include "parser.h"
#include "scanner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static TokenType current_token;
static char current_token_str[MAX_TOKEN_LENGTH];
static int current_line;
static int current_column;
static bool trace_parse = FALSE;
static ASTContext* ast_context = NULL;
static int node_id_counter = 0;

typedef struct {
    TreeNode* first;
    TreeNode* last;
    int count;
} NodeList;

static void initNodeList(NodeList* list) {
    list->first = NULL;
    list->last = NULL;
    list->count = 0;
}

static void appendToNodeList(NodeList* list, TreeNode* node) {
    if (node == NULL) return;

    if (list->last != NULL) {
        list->last->sibling = node;
        node->parent = list->last->parent;
    } else {
        list->first = node;
    }

    list->last = node;
    list->count++;

    while (node->sibling != NULL) {
        node = node->sibling;
        list->count++;
    }
    list->last = node;
}

TreeNode* createNode(NodeKind kind) {
    TreeNode* node = (TreeNode*)malloc(sizeof(TreeNode));
    if (node == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for tree node\n");
        return NULL;
    }

    node->node_kind = kind;
    node->line_number = current_line;
    node->column = current_column;
    node->parent = NULL;
    node->sibling = NULL;
    node->num_children = 0;
    node->data_type = TYPE_UNKNOWN;
    node->token_type = TOKEN_UNKNOWN;

    for (int i = 0; i < MAX_CHILDREN; i++) {
        node->child[i] = NULL;
    }

    memset(node->name, 0, MAX_TOKEN_LENGTH);
    memset(node->value, 0, MAX_TOKEN_LENGTH);

    switch (kind) {
        case NODE_IR:
            strcpy(node->name, "ir");
            break;
        case NODE_ATTRIBUTE_REPEATS:
            strcpy(node->name, "attribute_repeats");
            break;
        case NODE_ATTRIBUTE_DEF:
            strcpy(node->name, "attribute_def");
            break;
        case NODE_VALUE_DEF:
            strcpy(node->name, "value_def");
            break;
        case NODE_VALUE_INFO_DEF:
            strcpy(node->name, "value_info_def");
            break;
        case NODE_TYPE_DEF:
            strcpy(node->name, "type_def");
            break;
        case NODE_TENSOR_TYPE_DEF:
            strcpy(node->name, "tensor_type_def");
            break;
        case NODE_ELEM_TYPE_DEF:
            strcpy(node->name, "elem_type_def");
            break;
        case NODE_SHAPE_DEF:
            strcpy(node->name, "shape_def");
            break;
        case NODE_DIM_LIST:
            strcpy(node->name, "dim_list");
            break;
        case NODE_DIM_REPEATS:
            strcpy(node->name, "dim_repeats");
            break;
        case NODE_DIM_DEF:
            strcpy(node->name, "dim_def");
            break;
        case NODE_TENSOR_DEF:
            strcpy(node->name, "tensor_def");
            break;
        case NODE_DATA_TYPE_DEF:
            strcpy(node->name, "data_type_def");
            break;
        case NODE_DIMS_DEF:
            strcpy(node->name, "dims_def");
            break;
        case NODE_RAW_DATA_DEF:
            strcpy(node->name, "raw_data_def");
            break;
        case NODE_OPSET_IMPORT_DEF:
            strcpy(node->name, "opset_import_def");
            break;
        case NODE_DOMAIN_DEF:
            strcpy(node->name, "domain_def");
            break;
        case NODE_VERSION_DEF:
            strcpy(node->name, "version_def");
            break;
        case NODE_INPUT_DEF:
            strcpy(node->name, "input_def");
            break;
        case NODE_OUTPUT_DEF:
            strcpy(node->name, "output_def");
            break;
        case NODE_INITIALIZER_DEF:
            strcpy(node->name, "initializer_def");
            break;
        case NODE_VALUE_INFO_LIST:
            strcpy(node->name, "value_info_list");
            break;
        case NODE_IDENTIFIER:
            strcpy(node->name, "identifier");
            break;
        case NODE_INTEGER:
            strcpy(node->name, "integer");
            break;
        case NODE_FLOAT_NUM:
            strcpy(node->name, "float_number");
            break;
        case NODE_STRING_LIT:
            strcpy(node->name, "string_literal");
            break;
        case NODE_BOOL_LIT:
            strcpy(node->name, "bool_literal");
            break;
        case NODE_BYTES_LIT:
            strcpy(node->name, "bytes_literal");
            break;
        case NODE_DIM_VALUE_NODE:
            strcpy(node->name, "dim_value");
            break;
        case NODE_DIM_PARAM_NODE:
            strcpy(node->name, "dim_param");
            break;
        case NODE_PRODUCER_DEF:
            strcpy(node->name, "producer_def");
            break;
        case NODE_DOC_STRING_DEF:
            strcpy(node->name, "doc_string_def");
            break;
        case NODE_MODEL_VER_DEF:
            strcpy(node->name, "model_version_def");
            break;
        case NODE_ROOT:
            strcpy(node->name, "root");
            break;
        case NODE_EXPRESSION:
            strcpy(node->name, "expression");
            break;
        default:
            strcpy(node->name, "unknown");
            break;
    }

    node_id_counter++;

    return node;
}

void addChild(TreeNode* parent, TreeNode* child) {
    if (parent == NULL || child == NULL) {
        return;
    }

    if (parent->num_children >= MAX_CHILDREN) {
        fprintf(stderr, "Error: Maximum number of children exceeded\n");
        return;
    }

    child->parent = parent;
    parent->child[parent->num_children] = child;
    parent->num_children++;
}

void addSibling(TreeNode* node, TreeNode* sibling) {
    if (node == NULL || sibling == NULL) {
        return;
    }

    TreeNode* current = node;
    while (current->sibling != NULL) {
        current = current->sibling;
    }
    current->sibling = sibling;
}

void printTree(TreeNode* node) {
    printTreeWithIndent(node, 0);
}

void printTreeWithIndent(TreeNode* node, int indent) {
    if (node == NULL) {
        return;
    }

    for (int i = 0; i < indent; i++) {
        fprintf(listing, "  ");
    }

    fprintf(listing, "%s", node->name);

    if (node->value[0] != '\0') {
        fprintf(listing, " : %s", node->value);
    }

    if (node->token_type != TOKEN_UNKNOWN) {
        fprintf(listing, " [token=%s]", tokenToString(node->token_type));
    }

    if (node->data_type != TYPE_UNKNOWN) {
        fprintf(listing, " [type=%s]", dataTypeToString(node->data_type));
    }

    fprintf(listing, " (line:%d, col:%d)", node->line_number, node->column);
    fprintf(listing, "\n");

    for (int i = 0; i < node->num_children; i++) {
        printTreeWithIndent(node->child[i], indent + 1);
    }

    printTreeWithIndent(node->sibling, indent);
}

void freeTree(TreeNode* node) {
    if (node == NULL) {
        return;
    }

    for (int i = 0; i < node->num_children; i++) {
        freeTree(node->child[i]);
    }

    freeTree(node->sibling);

    free(node);
}

TreeNode* getChild(TreeNode* node, int index) {
    if (node == NULL || index < 0 || index >= node->num_children) {
        return NULL;
    }
    return node->child[index];
}

int getChildCount(TreeNode* node) {
    if (node == NULL) {
        return 0;
    }
    return node->num_children;
}

TokenType getCurrentToken(void) {
    return current_token;
}

const char* getCurrentTokenString(void) {
    return current_token_str;
}

void advanceToken(void) {
    current_token = getToken();
    strncpy(current_token_str, current_token_string, MAX_TOKEN_LENGTH - 1);
    current_token_str[MAX_TOKEN_LENGTH - 1] = '\0';
    current_line = getCurrentLine();
    current_column = getCurrentColumn();
}

void expectToken(TokenType token) {
    if (current_token != token) {
        fprintf(listing, "Error: Expected token %s but got %s at line %d, column %d\n",
                tokenToString(token), tokenToString(current_token), current_line, current_column);
        Error = TRUE;
    }
}

bool checkToken(TokenType token) {
    return current_token == token;
}

void matchToken(TokenType token) {
    if (current_token == token) {
        advanceToken();
    } else {
        fprintf(listing, "Error: Expected %s but got %s at line %d, column %d\n",
                tokenToString(token), tokenToString(current_token), current_line, current_column);
        Error = TRUE;
    }
}

void matchTokenWithRecovery(TokenType token, const char* error_message) {
    if (current_token == token) {
        advanceToken();
    } else {
        fprintf(listing, "Error: %s at line %d, column %d\n", error_message, current_line, current_column);
        Error = TRUE;
        synchronizeParser();
    }
}

void synchronizeParser(void) {
    while (current_token != TOKEN_EOF) {
        if (current_token == TOKEN_LBRACE || current_token == TOKEN_RBRACE ||
            current_token == TOKEN_LBRACKET || current_token == TOKEN_RBRACKET ||
            current_token == TOKEN_SEMICOLON || current_token == TOKEN_COMMA) {
            advanceToken();
            return;
        }
        advanceToken();
    }
}

void initParser(FILE* source_file) {
    initScanner();
    setSourceFile(source_file);
    advanceToken();
    Error = FALSE;
}

void setTraceParse(bool trace) {
    trace_parse = trace;
}

void printParseTree(TreeNode* root) {
    if (listing != NULL) {
        fprintf(listing, "\n============= PARSE TREE =============\n");
        printTree(root);
        fprintf(listing, "======================================\n\n");
    }
}

ASTContext* getASTContext(void) {
    return ast_context;
}

void setASTContext(ASTContext* context) {
    ast_context = context;
}

void reportSyntaxError(const char* format, ...) {
    va_list args;
    va_start(args, format);

    fprintf(listing, "Syntax Error at line %d, column %d: ", current_line, current_column);
    vfprintf(listing, format, args);
    fprintf(listing, "\n");

    va_end(args);
    Error = TRUE;
}

void reportSyntaxErrorAt(int line, int col, const char* format, ...) {
    va_list args;
    va_start(args, format);

    fprintf(listing, "Syntax Error at line %d, column %d: ", line, col);
    vfprintf(listing, format, args);
    fprintf(listing, "\n");

    va_end(args);
    Error = TRUE;
}

TreeNode* parse(FILE* file) {
    initParser(file);
    TreeNode* root = parseSOnnxModel();

    if (current_token != TOKEN_EOF) {
        reportSyntaxError("Unexpected token %s after parsing complete", tokenToString(current_token));
    }

    return root;
}

TreeNode* parseSOnnxModel(void) {
    TreeNode* root = createNode(NODE_ROOT);

    while (current_token != TOKEN_EOF) {
        TreeNode* stmt = NULL;

        switch (current_token) {
            case TOKEN_IR:
                stmt = parseIrDef();
                break;
            case TOKEN_PRODUCER_NAME:
                stmt = parseProducerDef();
                break;
            case TOKEN_DOC_STRING:
                stmt = parseDocStringDef();
                break;
            case TOKEN_MODEL_VERSION:
                stmt = parseModelVersionDef();
                break;
            case TOKEN_INPUT:
                stmt = parseInputDef();
                break;
            case TOKEN_OUTPUT:
                stmt = parseOutputDef();
                break;
            case TOKEN_INITIALIZER:
                stmt = parseInitializerDef();
                break;
            case TOKEN_VALUE_INFO:
                stmt = parseValueInfoDef();
                break;
            case TOKEN_ATTRIBUTE:
                stmt = parseAttributeRepeats();
                break;
            case TOKEN_OPSET_IMPORT:
                stmt = parseOpsetImportDef();
                break;
            default:
                reportSyntaxError("Unexpected token %s at top level", tokenToString(current_token));
                advanceToken();
                break;
        }

        if (stmt != NULL) {
            addSibling(root, stmt);
        }
    }

    return root;
}

TreeNode* parseIrDef(void) {
    TreeNode* node = createNode(NODE_IR);
    matchToken(TOKEN_IR);
    matchToken(TOKEN_LBRACE);

    while (current_token == TOKEN_NAME || current_token == TOKEN_VER ||
           current_token == TOKEN_PRODUCER_NAME || current_token == TOKEN_PRODUCER_VERSION ||
           current_token == TOKEN_DOMAIN || current_token == TOKEN_MODEL_VERSION ||
           current_token == TOKEN_DOC_STRING || current_token == TOKEN_INPUT ||
           current_token == TOKEN_OUTPUT || current_token == TOKEN_INITIALIZER ||
           current_token == TOKEN_VALUE_INFO || current_token == TOKEN_ATTRIBUTE ||
           current_token == TOKEN_OPSET_IMPORT) {

        TreeNode* child = NULL;

        switch (current_token) {
            case TOKEN_NAME:
                child = createNode(NODE_IDENTIFIER);
                matchToken(TOKEN_NAME);
                matchToken(TOKEN_ASSIGN);
                strcpy(child->value, current_token_str);
                child->token_type = TOKEN_IDENTIFIER;
                matchToken(TOKEN_STRING_LITERAL);
                break;
            case TOKEN_VER:
                child = createNode(NODE_INTEGER);
                matchToken(TOKEN_VER);
                matchToken(TOKEN_ASSIGN);
                strcpy(child->value, current_token_str);
                child->token_type = TOKEN_INTEGER;
                matchToken(TOKEN_INTEGER);
                break;
            case TOKEN_PRODUCER_NAME:
                child = parseProducerDef();
                break;
            case TOKEN_PRODUCER_VERSION:
                child = createNode(NODE_PRODUCER_DEF);
                matchToken(TOKEN_PRODUCER_VERSION);
                matchToken(TOKEN_ASSIGN);
                strcpy(child->value, current_token_str);
                matchToken(TOKEN_STRING_LITERAL);
                break;
            case TOKEN_DOMAIN:
                child = parseDomainDef();
                break;
            case TOKEN_MODEL_VERSION:
                child = parseModelVersionDef();
                break;
            case TOKEN_DOC_STRING:
                child = parseDocStringDef();
                break;
            case TOKEN_INPUT:
                child = parseInputDef();
                break;
            case TOKEN_OUTPUT:
                child = parseOutputDef();
                break;
            case TOKEN_INITIALIZER:
                child = parseInitializerDef();
                break;
            case TOKEN_VALUE_INFO:
                child = parseValueInfoDef();
                break;
            case TOKEN_ATTRIBUTE:
                child = parseAttributeRepeats();
                break;
            case TOKEN_OPSET_IMPORT:
                child = parseOpsetImportDef();
                break;
            default:
                advanceToken();
                break;
        }

        if (child != NULL) {
            addChild(node, child);
        }
    }

    matchToken(TOKEN_RBRACE);
    return node;
}

TreeNode* parseProducerDef(void) {
    TreeNode* node = createNode(NODE_PRODUCER_DEF);
    matchToken(TOKEN_PRODUCER_NAME);
    matchToken(TOKEN_ASSIGN);
    strcpy(node->value, current_token_str);
    matchToken(TOKEN_STRING_LITERAL);
    return node;
}

TreeNode* parseDocStringDef(void) {
    TreeNode* node = createNode(NODE_DOC_STRING_DEF);
    matchToken(TOKEN_DOC_STRING);
    matchToken(TOKEN_ASSIGN);
    strcpy(node->value, current_token_str);
    matchToken(TOKEN_STRING_LITERAL);
    return node;
}

TreeNode* parseModelVersionDef(void) {
    TreeNode* node = createNode(NODE_MODEL_VER_DEF);
    matchToken(TOKEN_MODEL_VERSION);
    matchToken(TOKEN_ASSIGN);
    strcpy(node->value, current_token_str);
    matchToken(TOKEN_INTEGER);
    return node;
}

TreeNode* parseInputDef(void) {
    TreeNode* node = createNode(NODE_INPUT_DEF);
    matchToken(TOKEN_INPUT);
    matchToken(TOKEN_LBRACE);

    while (current_token != TOKEN_RBRACE && current_token != TOKEN_EOF) {
        TreeNode* child = NULL;

        switch (current_token) {
            case TOKEN_NAME:
                child = createNode(NODE_IDENTIFIER);
                matchToken(TOKEN_NAME);
                matchToken(TOKEN_ASSIGN);
                strcpy(child->value, current_token_str);
                child->token_type = TOKEN_IDENTIFIER;
                matchToken(TOKEN_STRING_LITERAL);
                break;
            case TOKEN_TYPE:
                child = parseTypeDef();
                break;
            default:
                reportSyntaxError("Unexpected token %s in input_def", tokenToString(current_token));
                advanceToken();
                break;
        }

        if (child != NULL) {
            addChild(node, child);
        }
    }

    matchToken(TOKEN_RBRACE);
    return node;
}

TreeNode* parseOutputDef(void) {
    TreeNode* node = createNode(NODE_OUTPUT_DEF);
    matchToken(TOKEN_OUTPUT);
    matchToken(TOKEN_LBRACE);

    while (current_token != TOKEN_RBRACE && current_token != TOKEN_EOF) {
        TreeNode* child = NULL;

        switch (current_token) {
            case TOKEN_NAME:
                child = createNode(NODE_IDENTIFIER);
                matchToken(TOKEN_NAME);
                matchToken(TOKEN_ASSIGN);
                strcpy(child->value, current_token_str);
                child->token_type = TOKEN_IDENTIFIER;
                matchToken(TOKEN_STRING_LITERAL);
                break;
            case TOKEN_TYPE:
                child = parseTypeDef();
                break;
            default:
                reportSyntaxError("Unexpected token %s in output_def", tokenToString(current_token));
                advanceToken();
                break;
        }

        if (child != NULL) {
            addChild(node, child);
        }
    }

    matchToken(TOKEN_RBRACE);
    return node;
}

TreeNode* parseInitializerDef(void) {
    TreeNode* node = createNode(NODE_INITIALIZER_DEF);
    matchToken(TOKEN_INITIALIZER);
    matchToken(TOKEN_LBRACE);

    while (current_token != TOKEN_RBRACE && current_token != TOKEN_EOF) {
        TreeNode* child = NULL;

        switch (current_token) {
            case TOKEN_NAME:
                child = createNode(NODE_IDENTIFIER);
                matchToken(TOKEN_NAME);
                matchToken(TOKEN_ASSIGN);
                strcpy(child->value, current_token_str);
                child->token_type = TOKEN_IDENTIFIER;
                matchToken(TOKEN_STRING_LITERAL);
                break;
            case TOKEN_TYPE:
                child = parseTypeDef();
                break;
            default:
                reportSyntaxError("Unexpected token %s in initializer_def", tokenToString(current_token));
                advanceToken();
                break;
        }

        if (child != NULL) {
            addChild(node, child);
        }
    }

    matchToken(TOKEN_RBRACE);
    return node;
}

TreeNode* parseValueInfoDef(void) {
    TreeNode* node = createNode(NODE_VALUE_INFO_DEF);
    matchToken(TOKEN_VALUE_INFO);
    matchToken(TOKEN_LBRACE);

    while (current_token != TOKEN_RBRACE && current_token != TOKEN_EOF) {
        TreeNode* child = NULL;

        switch (current_token) {
            case TOKEN_NAME:
                child = createNode(NODE_IDENTIFIER);
                matchToken(TOKEN_NAME);
                matchToken(TOKEN_ASSIGN);
                strcpy(child->value, current_token_str);
                child->token_type = TOKEN_IDENTIFIER;
                matchToken(TOKEN_STRING_LITERAL);
                break;
            case TOKEN_TYPE:
                child = parseTypeDef();
                break;
            default:
                reportSyntaxError("Unexpected token %s in value_info_def", tokenToString(current_token));
                advanceToken();
                break;
        }

        if (child != NULL) {
            addChild(node, child);
        }
    }

    matchToken(TOKEN_RBRACE);
    return node;
}

TreeNode* parseAttributeRepeats(void) {
    TreeNode* node = createNode(NODE_ATTRIBUTE_REPEATS);
    matchToken(TOKEN_ATTRIBUTE);
    matchToken(TOKEN_LBRACE);

    while (current_token != TOKEN_RBRACE && current_token != TOKEN_EOF) {
        TreeNode* attr = parseAttributeDef();
        if (attr != NULL) {
            addChild(node, attr);
        }
    }

    matchToken(TOKEN_RBRACE);
    return node;
}

TreeNode* parseAttributeDef(void) {
    TreeNode* node = createNode(NODE_ATTRIBUTE_DEF);
    matchToken(TOKEN_ATTRIBUTE);
    matchToken(TOKEN_LBRACE);

    while (current_token != TOKEN_RBRACE && current_token != TOKEN_EOF) {
        TreeNode* child = NULL;

        switch (current_token) {
            case TOKEN_NAME:
                child = createNode(NODE_IDENTIFIER);
                matchToken(TOKEN_NAME);
                matchToken(TOKEN_ASSIGN);
                strcpy(child->value, current_token_str);
                child->token_type = TOKEN_IDENTIFIER;
                matchToken(TOKEN_STRING_LITERAL);
                break;
            case TOKEN_VALUE:
                child = parseValueDef();
                break;
            default:
                reportSyntaxError("Unexpected token %s in attribute_def", tokenToString(current_token));
                advanceToken();
                break;
        }

        if (child != NULL) {
            addChild(node, child);
        }
    }

    matchToken(TOKEN_RBRACE);
    return node;
}

TreeNode* parseValueDef(void) {
    TreeNode* node = createNode(NODE_VALUE_DEF);
    matchToken(TOKEN_VALUE);
    matchToken(TOKEN_ASSIGN);

    if (current_token == TOKEN_INTEGER) {
        strcpy(node->value, current_token_str);
        node->data_type = TYPE_INT;
        matchToken(TOKEN_INTEGER);
    } else if (current_token == TOKEN_FLOAT_NUMBER) {
        strcpy(node->value, current_token_str);
        node->data_type = TYPE_FLOAT;
        matchToken(TOKEN_FLOAT_NUMBER);
    } else if (current_token == TOKEN_STRING_LITERAL) {
        strcpy(node->value, current_token_str);
        node->data_type = TYPE_STRING;
        matchToken(TOKEN_STRING_LITERAL);
    } else if (current_token == TOKEN_BOOL_LITERAL) {
        strcpy(node->value, current_token_str);
        node->data_type = TYPE_BOOL;
        matchToken(TOKEN_BOOL_LITERAL);
    } else {
        reportSyntaxError("Unexpected token %s in value_def", tokenToString(current_token));
        advanceToken();
    }

    return node;
}

TreeNode* parseValueInfoListDef(void) {
    TreeNode* node = createNode(NODE_VALUE_INFO_LIST);
    matchToken(TOKEN_LBRACKET);

    while (current_token != TOKEN_RBRACKET && current_token != TOKEN_EOF) {
        TreeNode* info = parseValueInfoDef();
        if (info != NULL) {
            addChild(node, info);
        }

        if (current_token == TOKEN_COMMA) {
            matchToken(TOKEN_COMMA);
        }
    }

    matchToken(TOKEN_RBRACKET);
    return node;
}

TreeNode* parseTypeDef(void) {
    TreeNode* node = createNode(NODE_TYPE_DEF);
    matchToken(TOKEN_TYPE);
    matchToken(TOKEN_ASSIGN);

    while (current_token != TOKEN_RBRACE && current_token != TOKEN_EOF) {
        TreeNode* child = NULL;

        switch (current_token) {
            case TOKEN_TENSOR_TYPE:
                child = parseTensorTypeDef();
                break;
            default:
                reportSyntaxError("Unexpected token %s in type_def", tokenToString(current_token));
                advanceToken();
                break;
        }

        if (child != NULL) {
            addChild(node, child);
        }
    }

    return node;
}

TreeNode* parseTensorTypeDef(void) {
    TreeNode* node = createNode(NODE_TENSOR_TYPE_DEF);
    matchToken(TOKEN_TENSOR_TYPE);
    matchToken(TOKEN_LBRACE);

    while (current_token != TOKEN_RBRACE && current_token != TOKEN_EOF) {
        TreeNode* child = NULL;

        switch (current_token) {
            case TOKEN_ELEM_TYPE:
                child = parseElemTypeDef();
                break;
            case TOKEN_SHAPE:
                child = parseShapeDef();
                break;
            default:
                reportSyntaxError("Unexpected token %s in tensor_type_def", tokenToString(current_token));
                advanceToken();
                break;
        }

        if (child != NULL) {
            addChild(node, child);
        }
    }

    matchToken(TOKEN_RBRACE);
    return node;
}

TreeNode* parseElemTypeDef(void) {
    TreeNode* node = createNode(NODE_ELEM_TYPE_DEF);
    matchToken(TOKEN_ELEM_TYPE);
    matchToken(TOKEN_ASSIGN);

    strcpy(node->value, current_token_str);

    if (current_token == TOKEN_INT) {
        node->data_type = TYPE_INT;
        matchToken(TOKEN_INT);
    } else if (current_token == TOKEN_FLOAT) {
        node->data_type = TYPE_FLOAT;
        matchToken(TOKEN_FLOAT);
    } else if (current_token == TOKEN_STRING) {
        node->data_type = TYPE_STRING;
        matchToken(TOKEN_STRING);
    } else if (current_token == TOKEN_BOOL) {
        node->data_type = TYPE_BOOL;
        matchToken(TOKEN_BOOL);
    } else {
        reportSyntaxError("Unexpected token %s in elem_type_def", tokenToString(current_token));
        advanceToken();
    }

    return node;
}

TreeNode* parseShapeDef(void) {
    TreeNode* node = createNode(NODE_SHAPE_DEF);
    matchToken(TOKEN_SHAPE);
    matchToken(TOKEN_LBRACE);

    while (current_token != TOKEN_RBRACE && current_token != TOKEN_EOF) {
        TreeNode* child = parseDimRepeats();
        if (child != NULL) {
            addChild(node, child);
        }
    }

    matchToken(TOKEN_RBRACE);
    return node;
}

TreeNode* parseDimList(void) {
    TreeNode* node = createNode(NODE_DIM_LIST);
    matchToken(TOKEN_LBRACKET);

    while (current_token != TOKEN_RBRACKET && current_token != TOKEN_EOF) {
        TreeNode* dim = parseDimDef();
        if (dim != NULL) {
            addChild(node, dim);
        }

        if (current_token == TOKEN_COMMA) {
            matchToken(TOKEN_COMMA);
        }
    }

    matchToken(TOKEN_RBRACKET);
    return node;
}

TreeNode* parseDimRepeats(void) {
    TreeNode* node = createNode(NODE_DIM_REPEATS);
    matchToken(TOKEN_DIM);
    matchToken(TOKEN_LBRACE);

    while (current_token != TOKEN_RBRACE && current_token != TOKEN_EOF) {
        TreeNode* child = parseDimDef();
        if (child != NULL) {
            addChild(node, child);
        }
    }

    matchToken(TOKEN_RBRACE);
    return node;
}

TreeNode* parseDimDef(void) {
    TreeNode* node = createNode(NODE_DIM_DEF);

    if (current_token == TOKEN_DIM_VALUE) {
        matchToken(TOKEN_DIM_VALUE);
        matchToken(TOKEN_ASSIGN);
        strcpy(node->value, current_token_str);
        node->data_type = TYPE_INT;
        matchToken(TOKEN_INTEGER);
    } else if (current_token == TOKEN_DIM_PARAM) {
        matchToken(TOKEN_DIM_PARAM);
        matchToken(TOKEN_ASSIGN);
        strcpy(node->value, current_token_str);
        node->data_type = TYPE_STRING;
        matchToken(TOKEN_STRING_LITERAL);
    } else {
        reportSyntaxError("Unexpected token %s in dim_def", tokenToString(current_token));
        advanceToken();
    }

    return node;
}

TreeNode* parseTensorDef(void) {
    TreeNode* node = createNode(NODE_TENSOR_DEF);
    matchToken(TOKEN_TENSOR);
    matchToken(TOKEN_LBRACE);

    while (current_token != TOKEN_RBRACE && current_token != TOKEN_EOF) {
        TreeNode* child = NULL;

        switch (current_token) {
            case TOKEN_NAME:
                child = createNode(NODE_IDENTIFIER);
                matchToken(TOKEN_NAME);
                matchToken(TOKEN_ASSIGN);
                strcpy(child->value, current_token_str);
                child->token_type = TOKEN_IDENTIFIER;
                matchToken(TOKEN_STRING_LITERAL);
                break;
            case TOKEN_DATA_TYPE:
                child = parseDataTypeDef();
                break;
            case TOKEN_DIMS:
                child = parseDimsDef();
                break;
            case TOKEN_RAW_DATA:
                child = parseRawDataDef();
                break;
            default:
                reportSyntaxError("Unexpected token %s in tensor_def", tokenToString(current_token));
                advanceToken();
                break;
        }

        if (child != NULL) {
            addChild(node, child);
        }
    }

    matchToken(TOKEN_RBRACE);
    return node;
}

TreeNode* parseDataTypeDef(void) {
    TreeNode* node = createNode(NODE_DATA_TYPE_DEF);
    matchToken(TOKEN_DATA_TYPE);
    matchToken(TOKEN_ASSIGN);

    strcpy(node->value, current_token_str);

    if (current_token == TOKEN_INT) {
        node->data_type = TYPE_INT;
        matchToken(TOKEN_INT);
    } else if (current_token == TOKEN_FLOAT) {
        node->data_type = TYPE_FLOAT;
        matchToken(TOKEN_FLOAT);
    } else if (current_token == TOKEN_STRING) {
        node->data_type = TYPE_STRING;
        matchToken(TOKEN_STRING);
    } else if (current_token == TOKEN_BOOL) {
        node->data_type = TYPE_BOOL;
        matchToken(TOKEN_BOOL);
    } else {
        reportSyntaxError("Unexpected token %s in data_type_def", tokenToString(current_token));
        advanceToken();
    }

    return node;
}

TreeNode* parseDimsDef(void) {
    TreeNode* node = createNode(NODE_DIMS_DEF);
    matchToken(TOKEN_DIMS);
    matchToken(TOKEN_ASSIGN);

    if (current_token == TOKEN_LBRACKET) {
        matchToken(TOKEN_LBRACKET);

        while (current_token != TOKEN_RBRACKET && current_token != TOKEN_EOF) {
            TreeNode* dim = createNode(NODE_INTEGER);
            strcpy(dim->value, current_token_str);
            dim->data_type = TYPE_INT;
            matchToken(TOKEN_INTEGER);
            addChild(node, dim);

            if (current_token == TOKEN_COMMA) {
                matchToken(TOKEN_COMMA);
            }
        }

        matchToken(TOKEN_RBRACKET);
    } else {
        TreeNode* dim = createNode(NODE_INTEGER);
        strcpy(dim->value, current_token_str);
        dim->data_type = TYPE_INT;
        matchToken(TOKEN_INTEGER);
        addChild(node, dim);
    }

    return node;
}

TreeNode* parseRawDataDef(void) {
    TreeNode* node = createNode(NODE_RAW_DATA_DEF);
    matchToken(TOKEN_RAW_DATA);
    matchToken(TOKEN_ASSIGN);
    strcpy(node->value, current_token_str);
    matchToken(TOKEN_BYTES);
    return node;
}

TreeNode* parseOpsetImportDef(void) {
    TreeNode* node = createNode(NODE_OPSET_IMPORT_DEF);
    matchToken(TOKEN_OPSET_IMPORT);
    matchToken(TOKEN_LBRACE);

    while (current_token != TOKEN_RBRACE && current_token != TOKEN_EOF) {
        TreeNode* child = NULL;

        switch (current_token) {
            case TOKEN_DOMAIN:
                child = parseDomainDef();
                break;
            case TOKEN_VERSION:
                child = parseVersionDef();
                break;
            default:
                reportSyntaxError("Unexpected token %s in opset_import_def", tokenToString(current_token));
                advanceToken();
                break;
        }

        if (child != NULL) {
            addChild(node, child);
        }
    }

    matchToken(TOKEN_RBRACE);
    return node;
}

TreeNode* parseDomainDef(void) {
    TreeNode* node = createNode(NODE_DOMAIN_DEF);
    matchToken(TOKEN_DOMAIN);
    matchToken(TOKEN_ASSIGN);
    strcpy(node->value, current_token_str);
    matchToken(TOKEN_STRING_LITERAL);
    return node;
}

TreeNode* parseVersionDef(void) {
    TreeNode* node = createNode(NODE_VERSION_DEF);
    matchToken(TOKEN_VERSION);
    matchToken(TOKEN_ASSIGN);
    strcpy(node->value, current_token_str);
    node->data_type = TYPE_INT;
    matchToken(TOKEN_INTEGER);
    return node;
}

TreeNode* parseIdentifier(void) {
    TreeNode* node = createNode(NODE_IDENTIFIER);
    strcpy(node->value, current_token_str);
    node->token_type = TOKEN_IDENTIFIER;
    matchToken(TOKEN_IDENTIFIER);
    return node;
}

TreeNode* parseInteger(void) {
    TreeNode* node = createNode(NODE_INTEGER);
    strcpy(node->value, current_token_str);
    node->data_type = TYPE_INT;
    matchToken(TOKEN_INTEGER);
    return node;
}

TreeNode* parseFloatNumber(void) {
    TreeNode* node = createNode(NODE_FLOAT_NUM);
    strcpy(node->value, current_token_str);
    node->data_type = TYPE_FLOAT;
    matchToken(TOKEN_FLOAT_NUMBER);
    return node;
}

TreeNode* parseStringLiteral(void) {
    TreeNode* node = createNode(NODE_STRING_LIT);
    strcpy(node->value, current_token_str);
    node->data_type = TYPE_STRING;
    matchToken(TOKEN_STRING_LITERAL);
    return node;
}

TreeNode* parseBoolLiteral(void) {
    TreeNode* node = createNode(NODE_BOOL_LIT);
    strcpy(node->value, current_token_str);
    node->data_type = TYPE_BOOL;
    matchToken(TOKEN_BOOL_LITERAL);
    return node;
}

TreeNode* parseBytesLiteral(void) {
    TreeNode* node = createNode(NODE_BYTES_LIT);
    strcpy(node->value, current_token_str);
    matchToken(TOKEN_BYTES);
    return node;
}

TreeNode* parseExpression(void) {
    TreeNode* node = createNode(NODE_EXPRESSION);
    return node;
}
