#include "globals.h"
#include "scanner.h"
#include "parser.h"
#include "symboltable.h"
#include "semantic.h"
#include "tac.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILENAME 256

int EchoSource = FALSE;
int TraceScan = FALSE;
int TraceParse = FALSE;
int TraceAnalyze = TRUE;
int TraceCode = TRUE;
int Error = FALSE;

FILE* source;
FILE* listing;
FILE* code_output;
FILE* tac_output;

int lineno = 0;

char current_token_string[MAX_TOKEN_LENGTH];

static void printUsage(const char* program_name) {
    printf("Usage: %s [options] <source_file>\n", program_name);
    printf("Options:\n");
    printf("  -o <output_file>   Specify output file (default: stdout)\n");
    printf("  -e <echo_file>     Echo source to file\n");
    printf("  -t                 Enable trace scanning\n");
    printf("  -p                 Enable trace parsing\n");
    printf("  -a                 Enable trace analysis\n");
    printf("  -c                 Enable trace code generation\n");
    printf("  -h                 Display this help message\n");
}

static void parseArguments(int argc, char** argv, char** source_file, char** output_file, char** echo_file) {
    *source_file = NULL;
    *output_file = NULL;
    *echo_file = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            *output_file = argv[++i];
        } else if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) {
            *echo_file = argv[++i];
        } else if (strcmp(argv[i], "-t") == 0) {
            TraceScan = TRUE;
        } else if (strcmp(argv[i], "-p") == 0) {
            TraceParse = TRUE;
        } else if (strcmp(argv[i], "-a") == 0) {
            TraceAnalyze = TRUE;
        } else if (strcmp(argv[i], "-c") == 0) {
            TraceCode = TRUE;
        } else if (strcmp(argv[i], "-h") == 0) {
            printUsage(argv[0]);
            exit(0);
        } else if (argv[i][0] != '-') {
            *source_file = argv[i];
        }
    }
}

static void printBanner(FILE* output) {
    fprintf(output, "\n");
    fprintf(output, "==============================================\n");
    fprintf(output, "       S-ONNX Compiler v1.0                   \n");
    fprintf(output, "       Compilation Principles Project        \n");
    fprintf(output, "==============================================\n");
    fprintf(output, "\n");
}

static void printCompilationStages(FILE* output) {
    fprintf(output, "Compilation Stages:\n");
    fprintf(output, "  1. Lexical Analysis (Scanner)\n");
    fprintf(output, "  2. Syntax Analysis (Parser)\n");
    fprintf(output, "  3. Semantic Analysis\n");
    fprintf(output, "  4. Intermediate Code Generation (TAC)\n");
    fprintf(output, "\n");
}

static void printSourceFile(FILE* source_file, FILE* output) {
    if (source_file == NULL || output == NULL) {
        return;
    }

    fprintf(output, "============= SOURCE CODE =============\n");

    char buffer[MAX_LINE_LENGTH];
    int line_num = 1;

    rewind(source_file);

    while (fgets(buffer, MAX_LINE_LENGTH - 1, source_file) != NULL) {
        fprintf(output, "%4d: %s", line_num++, buffer);
    }

    fprintf(output, "========================================\n\n");

    rewind(source_file);
}

int main(int argc, char** argv) {
    char* source_file = NULL;
    char* output_file = NULL;
    char* echo_file = NULL;

    parseArguments(argc, argv, &source_file, &output_file, &echo_file);

    if (source_file == NULL) {
        fprintf(stderr, "Error: No source file specified\n");
        printUsage(argv[0]);
        return 1;
    }

    source = fopen(source_file, "r");
    if (source == NULL) {
        fprintf(stderr, "Error: Cannot open source file '%s'\n", source_file);
        return 1;
    }

    listing = (output_file != NULL) ? fopen(output_file, "w") : stdout;
    if (listing == NULL) {
        fprintf(stderr, "Error: Cannot open output file '%s'\n", output_file);
        fclose(source);
        return 1;
    }

    if (echo_file != NULL) {
        FILE* echo_output = fopen(echo_file, "w");
        if (echo_output != NULL) {
            printSourceFile(source, echo_output);
            fclose(echo_output);
        }
    }

    tac_output = listing;

    printBanner(listing);
    printCompilationStages(listing);

    fprintf(listing, "Compiling: %s\n\n", source_file);

    initErrorHandler();

    fprintf(listing, "============= STAGE 1: LEXICAL ANALYSIS =============\n");
    fprintf(listing, "\n");

    initScanner();
    setSourceFile(source);
    setTraceScan(TraceScan);

    TokenType token;
    int token_count = 0;

    fprintf(listing, "Tokens:\n");
    fprintf(listing, "------------------------------------------------\n");
    fprintf(listing, "%-6s %-6s %-6s %-s\n", "Line", "Col", "Type", "Value");
    fprintf(listing, "------------------------------------------------\n");

    do {
        token = getToken();
        if (token != TOKEN_EOF && token != TOKEN_ERROR) {
            fprintf(listing, "%-6d %-6d %-20s %s\n",
                    getCurrentLine(), getCurrentColumn(),
                    tokenToString(token), current_token_string);
            token_count++;
        } else if (token == TOKEN_ERROR) {
            reportLexicalError(getCurrentLine(), getCurrentColumn(),
                             "Illegal token: %s", current_token_string);
        }
    } while (token != TOKEN_EOF);

    fprintf(listing, "------------------------------------------------\n");
    fprintf(listing, "Total tokens: %d\n", token_count);
    fprintf(listing, "\n");

    if (hasLexicalErrors()) {
        fprintf(listing, "Lexical analysis completed with errors.\n");
        printErrorSummary();
        fclose(source);
        if (listing != stdout) fclose(listing);
        return 1;
    }

    fprintf(listing, "Lexical analysis completed successfully.\n");
    fprintf(listing, "\n");

    rewind(source);

    fprintf(listing, "============= STAGE 2: SYNTAX ANALYSIS =============\n");
    fprintf(listing, "\n");

    TreeNode* parse_tree = parse(source);

    if (Error || hasSyntaxErrors()) {
        fprintf(listing, "Syntax analysis completed with errors.\n");
        printErrorSummary();
        freeTree(parse_tree);
        fclose(source);
        if (listing != stdout) fclose(listing);
        return 1;
    }

    fprintf(listing, "Syntax analysis completed successfully.\n");
    fprintf(listing, "\n");

    if (TraceParse) {
        printParseTree(parse_tree);
    }

    fprintf(listing, "============= STAGE 3: SEMANTIC ANALYSIS =============\n");
    fprintf(listing, "\n");

    SymbolTable* symbol_table = createSymbolTable();

    SemanticResult semantic_result = analyzeSemantic(parse_tree, symbol_table);

    if (semantic_result != SEMANTIC_OK) {
        fprintf(listing, "Semantic analysis completed with errors.\n");
        printErrorSummary();
        destroySymbolTable(symbol_table);
        freeTree(parse_tree);
        fclose(source);
        if (listing != stdout) fclose(listing);
        return 1;
    }

    if (hasSemanticErrors()) {
        fprintf(listing, "Semantic analysis completed with errors.\n");
        printErrorSummary();
        destroySymbolTable(symbol_table);
        freeTree(parse_tree);
        fclose(source);
        if (listing != stdout) fclose(listing);
        return 1;
    }

    fprintf(listing, "Semantic analysis completed successfully.\n");
    fprintf(listing, "\n");

    printSymbolTableDetailed(symbol_table);

    fprintf(listing, "============= STAGE 4: TAC GENERATION =============\n");
    fprintf(listing, "\n");

    TACGenerator* tac_generator = createTACGenerator();
    setTraceTACGeneration(TraceCode);

    generateTACFromAST(tac_generator, parse_tree, symbol_table);

    fprintf(listing, "TAC generation completed.\n");
    fprintf(listing, "Total TAC instructions: %d\n", getTACInstructionCount(tac_generator));
    fprintf(listing, "\n");

    printTAC(tac_generator, listing);

    fprintf(listing, "============= COMPILATION SUMMARY =============\n");
    fprintf(listing, "\n");
    fprintf(listing, "Source File: %s\n", source_file);
    fprintf(listing, "Total Tokens: %d\n", token_count);
    fprintf(listing, "Lexical Errors: %d\n", getLexicalErrorCount());
    fprintf(listing, "Syntax Errors: %d\n", getSyntaxErrorCount());
    fprintf(listing, "Semantic Errors: %d\n", getSemanticErrorCount());
    fprintf(listing, "TAC Instructions: %d\n", getTACInstructionCount(tac_generator));
    fprintf(listing, "\n");

    if (getErrorCount() == 0) {
        fprintf(listing, "Compilation completed successfully!\n");
    } else {
        fprintf(listing, "Compilation completed with %d error(s).\n", getErrorCount());
        printErrorSummary();
    }

    fprintf(listing, "===============================================\n");

    destroyTACGenerator(tac_generator);
    destroySymbolTable(symbol_table);
    freeTree(parse_tree);
    fclose(source);
    if (listing != stdout) fclose(listing);

    return getErrorCount() > 0 ? 1 : 0;
}
