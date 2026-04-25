#ifndef _ERROR_H_
#define _ERROR_H_

#include "globals.h"

void initErrorHandler(void);
void reportError(ErrorType type, int line, int column, const char* format, ...);
void reportLexicalError(int line, int column, const char* format, ...);
void reportSyntaxErrorMsg(int line, int column, const char* format, ...);
void reportSemanticError(int line, int column, const char* format, ...);
void reportUnknownError(int line, int column, const char* format, ...);
void printErrorSummary(void);
void printErrorList(void);
void clearErrors(void);
int getErrorCount(void);
int getLexicalErrorCount(void);
int getSyntaxErrorCount(void);
int getSemanticErrorCount(void);
bool hasErrors(void);
bool hasLexicalErrors(void);
bool hasSyntaxErrors(void);
bool hasSemanticErrors(void);
ErrorInfo* getErrorList(void);
void setMaxErrors(int max);
int getMaxErrors(void);

typedef struct ErrorHandlerImplementation ErrorHandlerImplementation;
ErrorHandlerImplementation* getErrorHandler(void);

#endif
