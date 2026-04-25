#include "scanner.h"
#include "globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define MAX_LINE_BUFFER 4096
#define MAX_TOKEN_QUEUE 16

typedef struct ScannerImplementation {
    FILE* source_file;
    ScannerContext context;
    char line_buffer[MAX_LINE_BUFFER];
    int line_buffer_size;
    int line_buffer_pos;
    bool end_of_file;
    TokenInfo token_queue[MAX_TOKEN_QUEUE];
    int token_queue_size;
    int token_queue_front;
    int token_queue_rear;
    bool is_initialized;
    int unget_buffer_count;
    char unget_buffer[MAX_TOKEN_QUEUE][MAX_TOKEN_LENGTH];
    TokenType unget_token_types[MAX_TOKEN_QUEUE];
    int unget_line_numbers[MAX_TOKEN_QUEUE];
    int unget_columns[MAX_TOKEN_QUEUE];
} ScannerImplementation;

static ScannerImplementation scanner_impl = {
    .source_file = NULL,
    .context = {
        .state = STATE_START,
        .line_number = 1,
        .column = 1,
        .token_start_line = 1,
        .token_start_column = 1,
        .token_buffer = {0},
        .token_buffer_pos = 0,
        .save_to_buffer = TRUE,
        .comment_nesting = FALSE
    },
    .line_buffer = {0},
    .line_buffer_size = 0,
    .line_buffer_pos = 0,
    .end_of_file = FALSE,
    .token_queue = {0},
    .token_queue_size = 0,
    .token_queue_front = 0,
    .token_queue_rear = 0,
    .is_initialized = FALSE,
    .unget_buffer_count = 0,
    .unget_buffer = {0},
    .unget_token_types = {0},
    .unget_line_numbers = {0},
    .unget_columns = {0}
};

ScannerImplementation* getScannerImpl(void) {
    return &scanner_impl;
}

static const char* reserved_words[] = {
    "ir", "attribute", "tensor", "opset_import",
    "name", "ver", "producer_name", "producer_version",
    "domain", "model_version", "doc_string",
    "input", "output", "initializer", "value_info",
    "type", "dims", "raw_data", "data_type",
    "shape", "dim", "dim_value", "dim_param",
    "int", "float", "string", "bool",
    "opset", "version", "tensor_type", "elem_type",
    "value"
};

static const TokenType reserved_word_tokens[] = {
    TOKEN_IR, TOKEN_ATTRIBUTE, TOKEN_TENSOR, TOKEN_OPSET_IMPORT,
    TOKEN_NAME, TOKEN_VER, TOKEN_PRODUCER_NAME, TOKEN_PRODUCER_VERSION,
    TOKEN_DOMAIN, TOKEN_MODEL_VERSION, TOKEN_DOC_STRING,
    TOKEN_INPUT, TOKEN_OUTPUT, TOKEN_INITIALIZER, TOKEN_VALUE_INFO,
    TOKEN_TYPE, TOKEN_DIMS, TOKEN_RAW_DATA, TOKEN_DATA_TYPE,
    TOKEN_SHAPE, TOKEN_DIM, TOKEN_DIM_VALUE, TOKEN_DIM_PARAM,
    TOKEN_INT, TOKEN_FLOAT, TOKEN_STRING, TOKEN_BOOL,
    TOKEN_OPset, TOKEN_VERSION, TOKEN_TENSOR_TYPE, TOKEN_ELEM_TYPE,
    TOKEN_VALUE
};

static const int num_reserved_words = sizeof(reserved_words) / sizeof(reserved_words[0]);

void initScanner(void) {
    ScannerImplementation* impl = getScannerImpl();
    impl->context.state = STATE_START;
    impl->context.line_number = 1;
    impl->context.column = 1;
    impl->context.token_start_line = 1;
    impl->context.token_start_column = 1;
    impl->context.token_buffer_pos = 0;
    impl->context.save_to_buffer = TRUE;
    impl->context.comment_nesting = FALSE;
    impl->line_buffer_size = 0;
    impl->line_buffer_pos = 0;
    impl->end_of_file = FALSE;
    impl->token_queue_size = 0;
    impl->token_queue_front = 0;
    impl->token_queue_rear = 0;
    impl->is_initialized = TRUE;
    impl->unget_buffer_count = 0;
    memset(impl->line_buffer, 0, MAX_LINE_BUFFER);
    memset(impl->context.token_buffer, 0, MAX_TOKEN_LENGTH);
}

void setSourceFile(FILE* file) {
    ScannerImplementation* impl = getScannerImpl();
    impl->source_file = file;
    impl->end_of_file = FALSE;
}

static bool readNextLine(void) {
    ScannerImplementation* impl = getScannerImpl();
    if (impl->source_file == NULL) {
        return FALSE;
    }

    if (fgets(impl->line_buffer, MAX_LINE_BUFFER - 1, impl->source_file) != NULL) {
        impl->line_buffer_size = strlen(impl->line_buffer);
        impl->line_buffer_pos = 0;
        impl->context.line_number++;
        impl->context.column = 1;
        return TRUE;
    } else {
        impl->end_of_file = TRUE;
        return FALSE;
    }
}

char getNextChar(void) {
    ScannerImplementation* impl = getScannerImpl();

    if (impl->unget_buffer_count > 0) {
        impl->unget_buffer_count--;
        return EOF;
    }

    if (impl->line_buffer_pos >= impl->line_buffer_size) {
        if (!readNextLine()) {
            impl->end_of_file = TRUE;
            return EOF;
        }
    }

    char c = impl->line_buffer[impl->line_buffer_pos++];
    impl->context.column++;
    return c;
}

void ungetChar(void) {
    ScannerImplementation* impl = getScannerImpl();
    if (impl->line_buffer_pos > 0) {
        impl->line_buffer_pos--;
        impl->context.column--;
    }
}

int getCurrentLine(void) {
    return getScannerImpl()->context.line_number;
}

int getCurrentColumn(void) {
    return getScannerImpl()->context.column;
}

bool isReservedWord(const char* identifier) {
    for (int i = 0; i < num_reserved_words; i++) {
        if (strcmp(identifier, reserved_words[i]) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

TokenType reservedWordLookup(const char* word) {
    for (int i = 0; i < num_reserved_words; i++) {
        if (strcmp(word, reserved_words[i]) == 0) {
            return reserved_word_tokens[i];
        }
    }
    return TOKEN_IDENTIFIER;
}

const char* getTokenName(TokenType token) {
    switch (token) {
        case TOKEN_UNKNOWN: return "UNKNOWN";
        case TOKEN_IR: return "IR";
        case TOKEN_ATTRIBUTE: return "ATTRIBUTE";
        case TOKEN_TENSOR: return "TENSOR";
        case TOKEN_OPSET_IMPORT: return "OPSET_IMPORT";
        case TOKEN_NAME: return "NAME";
        case TOKEN_VER: return "VER";
        case TOKEN_PRODUCER_NAME: return "PRODUCER_NAME";
        case TOKEN_PRODUCER_VERSION: return "PRODUCER_VERSION";
        case TOKEN_DOMAIN: return "DOMAIN";
        case TOKEN_MODEL_VERSION: return "MODEL_VERSION";
        case TOKEN_DOC_STRING: return "DOC_STRING";
        case TOKEN_INPUT: return "INPUT";
        case TOKEN_OUTPUT: return "OUTPUT";
        case TOKEN_INITIALIZER: return "INITIALIZER";
        case TOKEN_VALUE_INFO: return "VALUE_INFO";
        case TOKEN_TYPE: return "TYPE";
        case TOKEN_DIMS: return "DIMS";
        case TOKEN_RAW_DATA: return "RAW_DATA";
        case TOKEN_DATA_TYPE: return "DATA_TYPE";
        case TOKEN_SHAPE: return "SHAPE";
        case TOKEN_DIM: return "DIM";
        case TOKEN_DIM_VALUE: return "DIM_VALUE";
        case TOKEN_DIM_PARAM: return "DIM_PARAM";
        case TOKEN_INT: return "INT";
        case TOKEN_FLOAT: return "FLOAT";
        case TOKEN_STRING: return "STRING";
        case TOKEN_BOOL: return "BOOL";
        case TOKEN_OPset: return "OPSET";
        case TOKEN_VERSION: return "VERSION";
        case TOKEN_TENSOR_TYPE: return "TENSOR_TYPE";
        case TOKEN_ELEM_TYPE: return "ELEM_TYPE";
        case TOKEN_VALUE: return "VALUE";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_INTEGER: return "INTEGER";
        case TOKEN_FLOAT_NUMBER: return "FLOAT_NUMBER";
        case TOKEN_STRING_LITERAL: return "STRING_LITERAL";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_COLON: return "COLON";
        case TOKEN_ASSIGN: return "ASSIGN";
        case TOKEN_QUESTION: return "QUESTION";
        case TOKEN_PIPE: return "PIPE";
        case TOKEN_AMPERSAND: return "AMPERSAND";
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_STAR: return "STAR";
        case TOKEN_SLASH: return "SLASH";
        case TOKEN_PERCENT: return "PERCENT";
        case TOKEN_LT: return "LT";
        case TOKEN_GT: return "GT";
        case TOKEN_LE: return "LE";
        case TOKEN_GE: return "GE";
        case TOKEN_EQ: return "EQ";
        case TOKEN_NE: return "NE";
        case TOKEN_AND: return "AND";
        case TOKEN_OR: return "OR";
        case TOKEN_NOT: return "NOT";
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        case TOKEN_BOOL_LITERAL: return "BOOL_LITERAL";
        case TOKEN_BYTES: return "BYTES";
        default: return "UNRECOGNIZED";
    }
}

void printTokenInfo(TokenType token, const char* token_string, int line, int column) {
    if (listing != NULL) {
        fprintf(listing, "%-4d%-4d: %-20s : %s\n", line, column, getTokenName(token), token_string);
    }
}

static void clearTokenBuffer(void) {
    ScannerImplementation* impl = getScannerImpl();
    impl->context.token_buffer_pos = 0;
    memset(impl->context.token_buffer, 0, MAX_TOKEN_LENGTH);
}

static void appendToTokenBuffer(char c) {
    ScannerImplementation* impl = getScannerImpl();
    if (impl->context.token_buffer_pos < MAX_TOKEN_LENGTH - 1) {
        impl->context.token_buffer[impl->context.token_buffer_pos++] = c;
    }
}

static const char* getTokenBuffer(void) {
    ScannerImplementation* impl = getScannerImpl();
    return impl->context.token_buffer;
}

static void startToken(void) {
    ScannerImplementation* impl = getScannerImpl();
    impl->context.token_start_line = impl->context.line_number;
    impl->context.token_start_column = impl->context.column;
    clearTokenBuffer();
}

static TokenType finishToken(TokenType token) {
    ScannerImplementation* impl = getScannerImpl();
    appendToTokenBuffer('\0');
    strncpy(current_token_string, getTokenBuffer(), MAX_TOKEN_LENGTH - 1);
    current_token_string[MAX_TOKEN_LENGTH - 1] = '\0';

    if (TraceScan && listing != NULL) {
        printTokenInfo(token, getTokenBuffer(), impl->context.token_start_line, impl->context.token_start_column);
    }

    return token;
}

static TokenType handleIdentifierOrReserved(void) {
    ScannerImplementation* impl = getScannerImpl();
    char* buffer = (char*)getTokenBuffer();

    if (isReservedWord(buffer)) {
        return finishToken(reservedWordLookup(buffer));
    }

    if (strcmp(buffer, "true") == 0 || strcmp(buffer, "false") == 0) {
        return finishToken(TOKEN_BOOL_LITERAL);
    }

    return finishToken(TOKEN_IDENTIFIER);
}

static TokenType handleSingleCharOperator(char c, TokenType single, TokenType error) {
    (void)c;
    return finishToken(single);
}

static TokenType handleTwoCharOperator(char first, char second, TokenType single, TokenType double_op) {
    ScannerImplementation* impl = getScannerImpl();
    char c = getNextChar();

    if (c == second) {
        appendToTokenBuffer(c);
        return finishToken(double_op);
    } else {
        ungetChar();
        return finishToken(single);
    }
}

static TokenType handleAssignOrEq(void) {
    ScannerImplementation* impl = getScannerImpl();
    char c = getNextChar();

    if (c == '=') {
        appendToTokenBuffer(c);
        return finishToken(TOKEN_EQ);
    } else {
        ungetChar();
        return finishToken(TOKEN_ASSIGN);
    }
}

static TokenType handleLtOrLe(void) {
    ScannerImplementation* impl = getScannerImpl();
    char c = getNextChar();

    if (c == '=') {
        appendToTokenBuffer(c);
        return finishToken(TOKEN_LE);
    } else if (c == '>') {
        appendToTokenBuffer(c);
        return finishToken(TOKEN_NE);
    } else {
        ungetChar();
        return finishToken(TOKEN_LT);
    }
}

static TokenType handleGtOrGe(void) {
    ScannerImplementation* impl = getScannerImpl();
    char c = getNextChar();

    if (c == '=') {
        appendToTokenBuffer(c);
        return finishToken(TOKEN_GE);
    } else {
        ungetChar();
        return finishToken(TOKEN_GT);
    }
}

static TokenType handleStringLiteral(void) {
    ScannerImplementation* impl = getScannerImpl();
    char c;

    while ((c = getNextChar()) != EOF) {
        if (c == '"') {
            if (impl->context.token_buffer_pos > 0 &&
                impl->context.token_buffer[impl->context.token_buffer_pos - 1] == '\\') {
                impl->context.token_buffer[impl->context.token_buffer_pos - 1] = '"';
            } else {
                return finishToken(TOKEN_STRING_LITERAL);
            }
        } else if (c == '\n') {
            impl->context.line_number++;
            impl->context.column = 0;
            appendToTokenBuffer(c);
        } else {
            appendToTokenBuffer(c);
        }
    }

    return finishToken(TOKEN_ERROR);
}

static TokenType handleBytesLiteral(void) {
    ScannerImplementation* impl = getScannerImpl();
    char c;
    int hex_count = 0;
    bool valid_hex = TRUE;

    while ((c = getNextChar()) != EOF) {
        if (c == '"') {
            if (impl->context.token_buffer_pos > 0 &&
                impl->context.token_buffer[impl->context.token_buffer_pos - 1] == '\\') {
                impl->context.token_buffer[impl->context.token_buffer_pos - 1] = '"';
            } else {
                if (!valid_hex || hex_count % 2 != 0) {
                    return finishToken(TOKEN_ERROR);
                }
                return finishToken(TOKEN_BYTES);
            }
        } else if (c == '\n') {
            impl->context.line_number++;
            impl->context.column = 0;
            appendToTokenBuffer(c);
            valid_hex = FALSE;
        } else if (isxdigit((unsigned char)c)) {
            appendToTokenBuffer(c);
            hex_count++;
        } else if (isspace((unsigned char)c) || c == '\\') {
            appendToTokenBuffer(c);
        } else {
            valid_hex = FALSE;
            appendToTokenBuffer(c);
        }
    }

    return finishToken(TOKEN_ERROR);
}

static TokenType handleSingleLineComment(void) {
    ScannerImplementation* impl = getScannerImpl();
    char c;

    while ((c = getNextChar()) != EOF) {
        if (c == '\n') {
            impl->context.line_number++;
            impl->context.column = 0;
            impl->context.state = STATE_START;
            break;
        }
    }

    return getToken();
}

static TokenType handleMultiLineComment(void) {
    ScannerImplementation* impl = getScannerImpl();
    char c;
    int nesting_level = 1;

    while ((c = getNextChar()) != EOF) {
        if (c == '/') {
            char next = getNextChar();
            if (next == '*') {
                nesting_level++;
            } else {
                ungetChar();
            }
        } else if (c == '*') {
            char next = getNextChar();
            if (next == '/') {
                nesting_level--;
                if (nesting_level == 0) {
                    impl->context.state = STATE_START;
                    break;
                }
            } else {
                ungetChar();
            }
        } else if (c == '\n') {
            impl->context.line_number++;
            impl->context.column = 0;
        }
    }

    if (nesting_level > 0) {
        return finishToken(TOKEN_ERROR);
    }

    return getToken();
}

TokenType getToken(void) {
    ScannerImplementation* impl = getScannerImpl();

    if (!impl->is_initialized) {
        initScanner();
    }

    if (impl->end_of_file) {
        strcpy(current_token_string, "EOF");
        return TOKEN_EOF;
    }

    impl->context.state = STATE_START;
    clearTokenBuffer();
    startToken();
    char c;

    while (impl->context.state != STATE_DONE) {
        c = getNextChar();

        if (c == EOF && impl->context.state != STATE_IN_STRING &&
            impl->context.state != STATE_IN_COMMENT &&
            impl->context.state != STATE_IN_MULTILINE_COMMENT &&
            impl->context.state != STATE_IN_BYTES) {
            if (impl->context.token_buffer_pos == 0) {
                strcpy(current_token_string, "EOF");
                return TOKEN_EOF;
            }
            return finishToken(TOKEN_EOF);
        }

        switch (impl->context.state) {
            case STATE_START:
                if (isalpha(c) || c == '_') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_IN_IDENTIFIER;
                }
                else if (isdigit(c)) {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_IN_INTEGER;
                }
                else if (c == '"') {
                    impl->context.state = STATE_IN_STRING;
                }
                else if (c == '\'') {
                    impl->context.state = STATE_IN_STRING;
                }
                else if (c == '`') {
                    impl->context.state = STATE_IN_BYTES;
                }
                else if (isspace(c)) {
                    impl->context.save_to_buffer = FALSE;
                    if (c == '\n') {
                        impl->context.line_number++;
                        impl->context.column = 0;
                    }
                }
                else if (c == '/') {
                    char next = getNextChar();
                    if (next == '/') {
                        impl->context.save_to_buffer = FALSE;
                        impl->context.state = STATE_IN_COMMENT;
                    } else if (next == '*') {
                        impl->context.save_to_buffer = FALSE;
                        impl->context.state = STATE_IN_MULTILINE_COMMENT;
                    } else {
                        ungetChar();
                        appendToTokenBuffer(c);
                        impl->context.state = STATE_DONE;
                        return finishToken(TOKEN_SLASH);
                    }
                }
                else if (c == '{') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_DONE;
                    return finishToken(TOKEN_LBRACE);
                }
                else if (c == '}') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_DONE;
                    return finishToken(TOKEN_RBRACE);
                }
                else if (c == '[') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_DONE;
                    return finishToken(TOKEN_LBRACKET);
                }
                else if (c == ']') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_DONE;
                    return finishToken(TOKEN_RBRACKET);
                }
                else if (c == '(') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_DONE;
                    return finishToken(TOKEN_LPAREN);
                }
                else if (c == ')') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_DONE;
                    return finishToken(TOKEN_RPAREN);
                }
                else if (c == ',') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_DONE;
                    return finishToken(TOKEN_COMMA);
                }
                else if (c == ';') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_DONE;
                    return finishToken(TOKEN_SEMICOLON);
                }
                else if (c == ':') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_DONE;
                    return finishToken(TOKEN_COLON);
                }
                else if (c == '=') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_ASSIGN;
                }
                else if (c == '<') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_LT;
                }
                else if (c == '>') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_GT;
                }
                else if (c == '!') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_EXCLAMATION;
                }
                else if (c == '|') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_PIPE;
                }
                else if (c == '&') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_AMPERSAND;
                }
                else if (c == '+') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_PLUS;
                }
                else if (c == '-') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_MINUS;
                }
                else if (c == '*') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_STAR;
                }
                else if (c == '%') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_PERCENT;
                }
                else if (c == '?') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_QUESTION;
                }
                else if (c == '.') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_DOT;
                }
                else {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_DONE;
                    return finishToken(TOKEN_ERROR);
                }
                break;

            case STATE_IN_IDENTIFIER:
                if (isalnum(c) || c == '_') {
                    appendToTokenBuffer(c);
                } else {
                    ungetChar();
                    impl->context.state = STATE_DONE;
                    return handleIdentifierOrReserved();
                }
                break;

            case STATE_IN_INTEGER:
                if (isdigit(c)) {
                    appendToTokenBuffer(c);
                } else if (c == '.') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_IN_FLOAT;
                } else if (c == 'e' || c == 'E') {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_IN_FLOAT;
                } else {
                    ungetChar();
                    impl->context.state = STATE_DONE;
                    return finishToken(TOKEN_INTEGER);
                }
                break;

            case STATE_IN_FLOAT:
                if (isdigit(c)) {
                    appendToTokenBuffer(c);
                } else if (c == 'e' || c == 'E') {
                    appendToTokenBuffer(c);
                    char next = getNextChar();
                    if (next == '+' || next == '-') {
                        appendToTokenBuffer(next);
                    } else {
                        ungetChar();
                    }
                } else {
                    ungetChar();
                    impl->context.state = STATE_DONE;
                    return finishToken(TOKEN_FLOAT_NUMBER);
                }
                break;

            case STATE_IN_STRING:
                if (c == '"') {
                    if (impl->context.token_buffer_pos > 0 &&
                        impl->context.token_buffer[impl->context.token_buffer_pos - 1] == '\\') {
                        impl->context.token_buffer[impl->context.token_buffer_pos - 1] = '"';
                    } else {
                        impl->context.state = STATE_DONE;
                        return finishToken(TOKEN_STRING_LITERAL);
                    }
                } else if (c == '\\') {
                    impl->context.state = STATE_IN_STRING_ESCAPE;
                } else if (c == '\n' || c == EOF) {
                    ungetChar();
                    impl->context.state = STATE_DONE;
                    return finishToken(TOKEN_ERROR);
                } else {
                    appendToTokenBuffer(c);
                }
                break;

            case STATE_IN_STRING_ESCAPE:
                if (c == 'n') {
                    appendToTokenBuffer('\n');
                } else if (c == 't') {
                    appendToTokenBuffer('\t');
                } else if (c == 'r') {
                    appendToTokenBuffer('\r');
                } else if (c == '\\') {
                    appendToTokenBuffer('\\');
                } else if (c == '"') {
                    appendToTokenBuffer('"');
                } else if (c == '\'') {
                    appendToTokenBuffer('\'');
                } else {
                    appendToTokenBuffer(c);
                }
                impl->context.state = STATE_IN_STRING;
                break;

            case STATE_IN_BYTES:
                if (c == '`') {
                    impl->context.state = STATE_DONE;
                    return handleBytesLiteral();
                } else if (c == '\n' || c == EOF) {
                    ungetChar();
                    impl->context.state = STATE_DONE;
                    return finishToken(TOKEN_ERROR);
                } else {
                    appendToTokenBuffer(c);
                }
                break;

            case STATE_IN_COMMENT:
                if (c == '\n') {
                    impl->context.state = STATE_START;
                }
                break;

            case STATE_IN_MULTILINE_COMMENT:
                if (c == '*') {
                    impl->context.state = STATE_MULTI_COMMENT_END_CHECK;
                } else if (c == EOF) {
                    impl->context.state = STATE_DONE;
                    return finishToken(TOKEN_ERROR);
                }
                break;

            case STATE_MULTI_COMMENT_END_CHECK:
                if (c == '/') {
                    impl->context.state = STATE_START;
                } else if (c == '*') {
                } else {
                    impl->context.state = STATE_IN_MULTILINE_COMMENT;
                }
                break;

            case STATE_ASSIGN:
                impl->context.state = STATE_DONE;
                return handleAssignOrEq();

            case STATE_LT:
                impl->context.state = STATE_DONE;
                return handleLtOrLe();

            case STATE_GT:
                impl->context.state = STATE_DONE;
                return handleGtOrGe();

            case STATE_EXCLAMATION:
                impl->context.state = STATE_DONE;
                return finishToken(TOKEN_NOT);

            case STATE_PIPE:
                impl->context.state = STATE_DONE;
                return handleTwoCharOperator('|', '|', TOKEN_PIPE, TOKEN_OR);

            case STATE_AMPERSAND:
                impl->context.state = STATE_DONE;
                return handleTwoCharOperator('&', '&', TOKEN_AMPERSAND, TOKEN_AND);

            case STATE_PLUS:
                impl->context.state = STATE_DONE;
                return handleSingleCharOperator(c, TOKEN_PLUS, TOKEN_ERROR);

            case STATE_MINUS:
                impl->context.state = STATE_DONE;
                return handleSingleCharOperator(c, TOKEN_MINUS, TOKEN_ERROR);

            case STATE_STAR:
                impl->context.state = STATE_DONE;
                return handleSingleCharOperator(c, TOKEN_STAR, TOKEN_ERROR);

            case STATE_PERCENT:
                impl->context.state = STATE_DONE;
                return handleSingleCharOperator(c, TOKEN_PERCENT, TOKEN_ERROR);

            case STATE_QUESTION:
                impl->context.state = STATE_DONE;
                return handleSingleCharOperator(c, TOKEN_QUESTION, TOKEN_ERROR);

            case STATE_DOT:
                if (isdigit(c)) {
                    appendToTokenBuffer(c);
                    impl->context.state = STATE_IN_FLOAT;
                } else {
                    ungetChar();
                    impl->context.state = STATE_DONE;
                    return finishToken(TOKEN_ERROR);
                }
                break;

            case STATE_DONE:
                break;
        }

        if (impl->context.state == STATE_DONE) {
            break;
        }
    }

    return finishToken(TOKEN_ERROR);
}

TokenInfo getTokenInfo(void) {
    TokenInfo info;
    info.token = getToken();
    strncpy(info.token_string, current_token_string, MAX_TOKEN_LENGTH - 1);
    info.line_number = getScannerImpl()->context.token_start_line;
    info.column = getScannerImpl()->context.token_start_column;
    return info;
}

void ungetToken(void) {
    ScannerImplementation* impl = getScannerImpl();
    if (impl->unget_buffer_count < MAX_TOKEN_QUEUE) {
        strncpy(impl->unget_buffer[impl->unget_buffer_count], current_token_string, MAX_TOKEN_LENGTH - 1);
        impl->unget_token_types[impl->unget_buffer_count] = getScannerImpl()->context.token_buffer_pos > 0 ?
            reservedWordLookup(current_token_string) : TOKEN_ERROR;
        impl->unget_line_numbers[impl->unget_buffer_count] = getScannerImpl()->context.token_start_line;
        impl->unget_columns[impl->unget_buffer_count] = getScannerImpl()->context.token_start_column;
        impl->unget_buffer_count++;
    }
}

void resetScanner(void) {
    ScannerImplementation* impl = getScannerImpl();
    initScanner();

    if (impl->source_file != NULL) {
        rewind(impl->source_file);
    }

    impl->end_of_file = FALSE;
    impl->token_queue_size = 0;
    impl->unget_buffer_count = 0;
}

void setEchoSource(bool echo) {
    EchoSource = echo;
}

void setTraceScan(bool trace) {
    TraceScan = trace;
}

const char* tokenToString(TokenType token) {
    return getTokenName(token);
}

const char* nodeKindToString(NodeKind kind) {
    switch (kind) {
        case NODE_ROOT: return "ROOT";
        case NODE_IR: return "IR";
        case NODE_ATTRIBUTE_REPEATS: return "ATTRIBUTE_REPEATS";
        case NODE_ATTRIBUTE_DEF: return "ATTRIBUTE_DEF";
        case NODE_VALUE_DEF: return "VALUE_DEF";
        case NODE_VALUE_INFO_DEF: return "VALUE_INFO_DEF";
        case NODE_TYPE_DEF: return "TYPE_DEF";
        case NODE_TENSOR_TYPE_DEF: return "TENSOR_TYPE_DEF";
        case NODE_ELEM_TYPE_DEF: return "ELEM_TYPE_DEF";
        case NODE_SHAPE_DEF: return "SHAPE_DEF";
        case NODE_DIM_LIST: return "DIM_LIST";
        case NODE_DIM_REPEATS: return "DIM_REPEATS";
        case NODE_DIM_DEF: return "DIM_DEF";
        case NODE_TENSOR_DEF: return "TENSOR_DEF";
        case NODE_DATA_TYPE_DEF: return "DATA_TYPE_DEF";
        case NODE_DIMS_DEF: return "DIMS_DEF";
        case NODE_RAW_DATA_DEF: return "RAW_DATA_DEF";
        case NODE_OPSET_IMPORT_DEF: return "OPSET_IMPORT_DEF";
        case NODE_DOMAIN_DEF: return "DOMAIN_DEF";
        case NODE_VERSION_DEF: return "VERSION_DEF";
        case NODE_INPUT_DEF: return "INPUT_DEF";
        case NODE_OUTPUT_DEF: return "OUTPUT_DEF";
        case NODE_INITIALIZER_DEF: return "INITIALIZER_DEF";
        case NODE_VALUE_INFO_LIST: return "VALUE_INFO_LIST";
        case NODE_IDENTIFIER: return "IDENTIFIER";
        case NODE_INTEGER: return "INTEGER";
        case NODE_FLOAT_NUM: return "FLOAT_NUM";
        case NODE_STRING_LIT: return "STRING_LIT";
        case NODE_BOOL_LIT: return "BOOL_LIT";
        case NODE_BYTES_LIT: return "BYTES_LIT";
        case NODE_DIM_VALUE_NODE: return "DIM_VALUE_NODE";
        case NODE_DIM_PARAM_NODE: return "DIM_PARAM_NODE";
        case NODE_PRODUCER_DEF: return "PRODUCER_DEF";
        case NODE_DOC_STRING_DEF: return "DOC_STRING_DEF";
        case NODE_MODEL_VER_DEF: return "MODEL_VER_DEF";
        case NODE_EXPRESSION: return "EXPRESSION";
        default: return "UNKNOWN_NODE";
    }
}

const char* dataTypeToString(DataType type) {
    switch (type) {
        case TYPE_UNKNOWN: return "UNKNOWN";
        case TYPE_INT: return "INT";
        case TYPE_FLOAT: return "FLOAT";
        case TYPE_STRING: return "STRING";
        case TYPE_BOOL: return "BOOL";
        case TYPE_TENSOR: return "TENSOR";
        case TYPE_DIM_LIST: return "DIM_LIST";
        case TYPE_OPSET_IMPORT: return "OPSET_IMPORT";
        default: return "UNRECOGNIZED_TYPE";
    }
}

const char* tacOpToString(TACOp op) {
    switch (op) {
        case TAC_OP_INPUT: return "INPUT";
        case TAC_OP_OUTPUT: return "OUTPUT";
        case TAC_OP_INITIALIZER: return "INITIALIZER";
        case TAC_OP_ATTRIBUTE: return "ATTRIBUTE";
        case TAC_OP_TENSOR: return "TENSOR";
        case TAC_OP_CONST: return "CONST";
        case TAC_OP_ADD: return "ADD";
        case TAC_OP_SUB: return "SUB";
        case TAC_OP_MUL: return "MUL";
        case TAC_OP_DIV: return "DIV";
        case TAC_OP_ASSIGN: return "ASSIGN";
        case TAC_OP_LOAD: return "LOAD";
        case TAC_OP_STORE: return "STORE";
        case TAC_OP_PARAM: return "PARAM";
        case TAC_OP_RETURN: return "RETURN";
        case TAC_OP_LABEL: return "LABEL";
        case TAC_OP_GOTO: return "GOTO";
        case TAC_OP_IF: return "IF";
        case TAC_OP_CALL: return "CALL";
        case TAC_OP_FUNC: return "FUNC";
        case TAC_OP_CONV: return "CONV";
        case TAC_OP_POOL: return "POOL";
        case TAC_OP_FC: return "FC";
        case TAC_OP_RELU: return "RELU";
        case TAC_OP_SIGMOID: return "SIGMOID";
        case TAC_OP_BN: return "BN";
        case TAC_OP_SOFTMAX: return "SOFTMAX";
        case TAC_OP_LRN: return "LRN";
        case TAC_OP_CONCAT: return "CONCAT";
        case TAC_OP_SLICE: return "SLICE";
        case TAC_OP_RESHAPE: return "RESHAPE";
        case TAC_OP_TRANSPOSE: return "TRANSPOSE";
        case TAC_OP_MAX: return "MAX";
        case TAC_OP_MIN: return "MIN";
        case TAC_OP_AVG: return "AVG";
        case TAC_OP_SUM: return "SUM";
        case TAC_OP_MATMUL: return "MATMUL";
        case TAC_OP_BIASADD: return "BIASADD";
        case TAC_OP_REDUCE: return "REDUCE";
        case TAC_OP_SQUEEZE: return "SQUEEZE";
        case TAC_OP_UNSQUEEZE: return "UNSQUEEZE";
        case TAC_OP_PAD: return "PAD";
        case TAC_OP_UPSAMPLE: return "UPSAMPLE";
        case TAC_OP_GEMM: return "GEMM";
        case TAC_OP_OPSET_IMPORT: return "OPSET_IMPORT";
        case TAC_OP_NOP: return "NOP";
        default: return "UNRECOGNIZED_OP";
    }
}

const char* errorTypeToString(ErrorType type) {
    switch (type) {
        case ERROR_LEXICAL: return "LEXICAL";
        case ERROR_SYNTAX: return "SYNTAX";
        case ERROR_SEMANTIC: return "SEMANTIC";
        case ERROR_UNKNOWN: return "UNKNOWN";
        default: return "UNRECOGNIZED_ERROR";
    }
}

void printToken(TokenType token, const char* token_string) {
    if (listing != NULL) {
        fprintf(listing, "%-15s : %s\n", getTokenName(token), token_string);
    }
}
