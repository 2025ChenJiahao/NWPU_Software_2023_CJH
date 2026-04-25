#include "error.h"
#include "scanner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define MAX_ERRORS 256

typedef struct ErrorHandlerImplementation {
    ErrorInfo* error_list;
    ErrorInfo* error_tail;
    int error_count;
    int lexical_error_count;
    int syntax_error_count;
    int semantic_error_count;
    int unknown_error_count;
    int max_errors;
    bool initialized;
} ErrorHandlerImplementation;

static ErrorHandlerImplementation error_handler = {
    .error_list = NULL,
    .error_tail = NULL,
    .error_count = 0,
    .lexical_error_count = 0,
    .syntax_error_count = 0,
    .semantic_error_count = 0,
    .unknown_error_count = 0,
    .max_errors = MAX_ERRORS,
    .initialized = FALSE
};

ErrorHandlerImplementation* getErrorHandler(void) {
    return &error_handler;
}

void initErrorHandler(void) {
    ErrorHandlerImplementation* handler = getErrorHandler();

    ErrorInfo* current = handler->error_list;
    while (current != NULL) {
        ErrorInfo* next = current->next;
        free(current);
        current = next;
    }

    handler->error_list = NULL;
    handler->error_tail = NULL;
    handler->error_count = 0;
    handler->lexical_error_count = 0;
    handler->syntax_error_count = 0;
    handler->semantic_error_count = 0;
    handler->unknown_error_count = 0;
    handler->initialized = TRUE;
}

static ErrorInfo* createErrorInfo(ErrorType type, int line, int column, const char* message) {
    ErrorInfo* info = (ErrorInfo*)malloc(sizeof(ErrorInfo));
    if (info == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for error info\n");
        return NULL;
    }

    info->type = type;
    info->line = line;
    info->column = column;
    info->next = NULL;

    if (message != NULL) {
        strncpy(info->message, message, MAX_ERROR_LENGTH - 1);
        info->message[MAX_ERROR_LENGTH - 1] = '\0';
    } else {
        info->message[0] = '\0';
    }

    info->token[0] = '\0';

    return info;
}

void reportError(ErrorType type, int line, int column, const char* format, ...) {
    ErrorHandlerImplementation* handler = getErrorHandler();

    if (!handler->initialized) {
        initErrorHandler();
    }

    if (handler->error_count >= handler->max_errors) {
        return;
    }

    va_list args;
    va_start(args, format);

    char message[MAX_ERROR_LENGTH];
    vsnprintf(message, MAX_ERROR_LENGTH - 1, format, args);
    message[MAX_ERROR_LENGTH - 1] = '\0';

    va_end(args);

    ErrorInfo* info = createErrorInfo(type, line, column, message);

    if (info == NULL) {
        return;
    }

    if (handler->error_list == NULL) {
        handler->error_list = info;
        handler->error_tail = info;
    } else {
        handler->error_tail->next = info;
        handler->error_tail = info;
    }

    handler->error_count++;

    switch (type) {
        case ERROR_LEXICAL:
            handler->lexical_error_count++;
            if (listing != NULL) {
                fprintf(listing, "[LEXICAL ERROR] Line %d, Column %d: %s\n", line, column, message);
            }
            break;
        case ERROR_SYNTAX:
            handler->syntax_error_count++;
            if (listing != NULL) {
                fprintf(listing, "[SYNTAX ERROR] Line %d, Column %d: %s\n", line, column, message);
            }
            break;
        case ERROR_SEMANTIC:
            handler->semantic_error_count++;
            if (listing != NULL) {
                fprintf(listing, "[SEMANTIC ERROR] Line %d, Column %d: %s\n", line, column, message);
            }
            break;
        case ERROR_UNKNOWN:
        default:
            handler->unknown_error_count++;
            if (listing != NULL) {
                fprintf(listing, "[UNKNOWN ERROR] Line %d, Column %d: %s\n", line, column, message);
            }
            break;
    }
}

void reportLexicalError(int line, int column, const char* format, ...) {
    va_list args;
    va_start(args, format);

    char message[MAX_ERROR_LENGTH];
    vsnprintf(message, MAX_ERROR_LENGTH - 1, format, args);
    message[MAX_ERROR_LENGTH - 1] = '\0';

    va_end(args);

    reportError(ERROR_LEXICAL, line, column, "%s", message);
}

void reportSyntaxErrorMsg(int line, int column, const char* format, ...) {
    va_list args;
    va_start(args, format);

    char message[MAX_ERROR_LENGTH];
    vsnprintf(message, MAX_ERROR_LENGTH - 1, format, args);
    message[MAX_ERROR_LENGTH - 1] = '\0';

    va_end(args);

    reportError(ERROR_SYNTAX, line, column, "%s", message);
}

void reportSemanticError(int line, int column, const char* format, ...) {
    va_list args;
    va_start(args, format);

    char message[MAX_ERROR_LENGTH];
    vsnprintf(message, MAX_ERROR_LENGTH - 1, format, args);
    message[MAX_ERROR_LENGTH - 1] = '\0';

    va_end(args);

    reportError(ERROR_SEMANTIC, line, column, "%s", message);
}

void reportUnknownError(int line, int column, const char* format, ...) {
    va_list args;
    va_start(args, format);

    char message[MAX_ERROR_LENGTH];
    vsnprintf(message, MAX_ERROR_LENGTH - 1, format, args);
    message[MAX_ERROR_LENGTH - 1] = '\0';

    va_end(args);

    reportError(ERROR_UNKNOWN, line, column, "%s", message);
}

void printErrorSummary(void) {
    ErrorHandlerImplementation* handler = getErrorHandler();

    if (listing == NULL) {
        return;
    }

    fprintf(listing, "\n============= ERROR SUMMARY =============\n");
    fprintf(listing, "Total Errors: %d\n", handler->error_count);
    fprintf(listing, "  Lexical Errors: %d\n", handler->lexical_error_count);
    fprintf(listing, "  Syntax Errors: %d\n", handler->syntax_error_count);
    fprintf(listing, "  Semantic Errors: %d\n", handler->semantic_error_count);
    fprintf(listing, "  Unknown Errors: %d\n", handler->unknown_error_count);
    fprintf(listing, "=========================================\n\n");
}

void printErrorList(void) {
    ErrorHandlerImplementation* handler = getErrorHandler();

    if (listing == NULL || handler->error_list == NULL) {
        return;
    }

    fprintf(listing, "\n============= ERROR LIST =============\n");

    ErrorInfo* current = handler->error_list;
    int error_num = 1;

    while (current != NULL) {
        fprintf(listing, "%d. [%s] Line %d, Column %d: %s\n",
                error_num++,
                errorTypeToString(current->type),
                current->line,
                current->column,
                current->message);

        if (current->token[0] != '\0') {
            fprintf(listing, "   Token: %s\n", current->token);
        }

        current = current->next;
    }

    fprintf(listing, "========================================\n\n");
}

void clearErrors(void) {
    initErrorHandler();
}

int getErrorCount(void) {
    return getErrorHandler()->error_count;
}

int getLexicalErrorCount(void) {
    return getErrorHandler()->lexical_error_count;
}

int getSyntaxErrorCount(void) {
    return getErrorHandler()->syntax_error_count;
}

int getSemanticErrorCount(void) {
    return getErrorHandler()->semantic_error_count;
}

bool hasErrors(void) {
    return getErrorHandler()->error_count > 0;
}

bool hasLexicalErrors(void) {
    return getErrorHandler()->lexical_error_count > 0;
}

bool hasSyntaxErrors(void) {
    return getErrorHandler()->syntax_error_count > 0;
}

bool hasSemanticErrors(void) {
    return getErrorHandler()->semantic_error_count > 0;
}

ErrorInfo* getErrorList(void) {
    return getErrorHandler()->error_list;
}

void setMaxErrors(int max) {
    getErrorHandler()->max_errors = max;
}

int getMaxErrors(void) {
    return getErrorHandler()->max_errors;
}
