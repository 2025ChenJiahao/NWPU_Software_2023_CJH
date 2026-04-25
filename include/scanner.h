#ifndef _SCANNER_H_
#define _SCANNER_H_

#include "globals.h"

typedef enum {
    STATE_START,
    STATE_IN_IDENTIFIER,
    STATE_IN_INTEGER,
    STATE_IN_FLOAT,
    STATE_IN_STRING,
    STATE_IN_COMMENT,
    STATE_IN_MULTILINE_COMMENT,
    STATE_IN_BYTES,
    STATE_ASSIGN,
    STATE_LT,
    STATE_GT,
    STATE_EXCLAMATION,
    STATE_PIPE,
    STATE_AMPERSAND,
    STATE_COLON,
    STATE_DOT,
    STATE_PLUS,
    STATE_MINUS,
    STATE_STAR,
    STATE_SLASH,
    STATE_PERCENT,
    STATE_QUESTION,
    STATE_IN_STRING_ESCAPE,
    STATE_IN_BYTES_HEX,
    STATE_MULTI_COMMENT_END_CHECK,
    STATE_DONE
} ScannerState;

typedef struct {
    ScannerState state;
    int line_number;
    int column;
    int token_start_line;
    int token_start_column;
    char token_buffer[MAX_TOKEN_LENGTH];
    int token_buffer_pos;
    bool save_to_buffer;
    bool comment_nesting;
} ScannerContext;

void initScanner(void);
void setSourceFile(FILE* file);
TokenType getToken(void);
TokenInfo getTokenInfo(void);
void ungetToken(void);
void resetScanner(void);
void setEchoSource(bool echo);
void setTraceScan(bool trace);
bool isReservedWord(const char* identifier);
TokenType reservedWordLookup(const char* word);
const char* getTokenName(TokenType token);
void printTokenInfo(TokenType token, const char* token_string, int line, int column);
char getNextChar(void);
void ungetChar(void);
int getCurrentLine(void);
int getCurrentColumn(void);

typedef struct ScannerImplementation ScannerImplementation;
ScannerImplementation* getScannerImpl(void);

#endif
