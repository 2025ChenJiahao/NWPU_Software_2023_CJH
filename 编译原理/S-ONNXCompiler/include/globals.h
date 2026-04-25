#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define MAX_TOKEN_LENGTH 256
#define MAX_IDENTIFIER_LENGTH 128
#define MAX_STRING_LENGTH 512
#define MAX_LINE_LENGTH 1024
#define MAX_ERROR_LENGTH 512
#define MAX_CHILDREN 32
#define MAX_SCOPE_DEPTH 64
#define HASH_TABLE_SIZE 256
#define MAX_TAC_INSTRUCTIONS 4096
#define MAX_DIMENSIONS 16

typedef enum {
    TOKEN_UNKNOWN = 0,
    TOKEN_IR,
    TOKEN_ATTRIBUTE,
    TOKEN_TENSOR,
    TOKEN_OPSET_IMPORT,
    TOKEN_NAME,
    TOKEN_VER,
    TOKEN_PRODUCER_NAME,
    TOKEN_PRODUCER_VERSION,
    TOKEN_DOMAIN,
    TOKEN_MODEL_VERSION,
    TOKEN_DOC_STRING,
    TOKEN_INPUT,
    TOKEN_OUTPUT,
    TOKEN_INITIALIZER,
    TOKEN_VALUE_INFO,
    TOKEN_TYPE,
    TOKEN_DIMS,
    TOKEN_RAW_DATA,
    TOKEN_DATA_TYPE,
    TOKEN_SHAPE,
    TOKEN_DIM,
    TOKEN_DIM_VALUE,
    TOKEN_DIM_PARAM,
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_BOOL,
    TOKEN_OPset,
    TOKEN_VERSION,
    TOKEN_TENSOR_TYPE,
    TOKEN_ELEM_TYPE,
    TOKEN_VALUE,
    TOKEN_RAW_DATA_TYPE,
    TOKEN_IDENTIFIER,
    TOKEN_INTEGER,
    TOKEN_FLOAT_NUMBER,
    TOKEN_STRING_LITERAL,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_COMMA,
    TOKEN_SEMICOLON,
    TOKEN_COLON,
    TOKEN_ASSIGN,
    TOKEN_QUESTION,
    TOKEN_PIPE,
    TOKEN_AMPERSAND,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LE,
    TOKEN_GE,
    TOKEN_EQ,
    TOKEN_NE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_EOF,
    TOKEN_ERROR,
    TOKEN_BOOL_LITERAL,
    TOKEN_BYTES
} TokenType;

typedef enum {
    NODE_ROOT,
    NODE_IR,
    NODE_ATTRIBUTE_REPEATS,
    NODE_ATTRIBUTE_DEF,
    NODE_VALUE_DEF,
    NODE_VALUE_INFO_DEF,
    NODE_TYPE_DEF,
    NODE_TENSOR_TYPE_DEF,
    NODE_ELEM_TYPE_DEF,
    NODE_SHAPE_DEF,
    NODE_DIM_LIST,
    NODE_DIM_REPEATS,
    NODE_DIM_DEF,
    NODE_TENSOR_DEF,
    NODE_DATA_TYPE_DEF,
    NODE_DIMS_DEF,
    NODE_RAW_DATA_DEF,
    NODE_OPSET_IMPORT_DEF,
    NODE_DOMAIN_DEF,
    NODE_VERSION_DEF,
    NODE_INPUT_DEF,
    NODE_OUTPUT_DEF,
    NODE_INITIALIZER_DEF,
    NODE_VALUE_INFO_LIST,
    NODE_IDENTIFIER,
    NODE_INTEGER,
    NODE_FLOAT_NUM,
    NODE_STRING_LIT,
    NODE_BOOL_LIT,
    NODE_BYTES_LIT,
    NODE_DIM_VALUE_NODE,
    NODE_DIM_PARAM_NODE,
    NODE_PRODUCER_DEF,
    NODE_DOC_STRING_DEF,
    NODE_MODEL_VER_DEF,
    NODE_EXPRESSION
} NodeKind;

typedef enum {
    TYPE_UNKNOWN,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_TENSOR,
    TYPE_DIM_LIST,
    TYPE_OPSET_IMPORT
} DataType;

typedef enum {
    TAC_OP_INPUT,
    TAC_OP_OUTPUT,
    TAC_OP_INITIALIZER,
    TAC_OP_ATTRIBUTE,
    TAC_OP_TENSOR,
    TAC_OP_CONST,
    TAC_OP_ADD,
    TAC_OP_SUB,
    TAC_OP_MUL,
    TAC_OP_DIV,
    TAC_OP_ASSIGN,
    TAC_OP_LOAD,
    TAC_OP_STORE,
    TAC_OP_PARAM,
    TAC_OP_RETURN,
    TAC_OP_LABEL,
    TAC_OP_GOTO,
    TAC_OP_IF,
    TAC_OP_CALL,
    TAC_OP_FUNC,
    TAC_OP_CONV,
    TAC_OP_POOL,
    TAC_OP_FC,
    TAC_OP_RELU,
    TAC_OP_SIGMOID,
    TAC_OP_BN,
    TAC_OP_SOFTMAX,
    TAC_OP_LRN,
    TAC_OP_CONCAT,
    TAC_OP_SLICE,
    TAC_OP_RESHAPE,
    TAC_OP_TRANSPOSE,
    TAC_OP_MAX,
    TAC_OP_MIN,
    TAC_OP_AVG,
    TAC_OP_SUM,
    TAC_OP_MATMUL,
    TAC_OP_BIASADD,
    TAC_OP_REDUCE,
    TAC_OP_SQUEEZE,
    TAC_OP_UNSQUEEZE,
    TAC_OP_PAD,
    TAC_OP_UPSAMPLE,
    TAC_OP_GEMM,
    TAC_OP_OPSET_IMPORT,
    TAC_OP_NOP
} TACOp;

typedef enum {
    ERROR_LEXICAL,
    ERROR_SYNTAX,
    ERROR_SEMANTIC,
    ERROR_UNKNOWN
} ErrorType;

typedef enum {
    TYPE_INT_VAL,
    TYPE_FLOAT_VAL,
    TYPE_STRING_VAL,
    TYPE_BOOL_VAL,
    TYPE_TENSOR_VAL,
    TYPE_INVALID
} ValueType;

typedef struct ErrorInfo {
    ErrorType type;
    int line;
    int column;
    char message[MAX_ERROR_LENGTH];
    char token[MAX_TOKEN_LENGTH];
    struct ErrorInfo* next;
} ErrorInfo;

typedef struct TACInstruction {
    int label;
    TACOp op;
    char result[MAX_TOKEN_LENGTH];
    char operand1[MAX_TOKEN_LENGTH];
    char operand2[MAX_TOKEN_LENGTH];
    char operand3[MAX_TOKEN_LENGTH];
    char comment[MAX_TOKEN_LENGTH];
    struct TACInstruction* next;
} TACInstruction;

typedef struct DimNode {
    int dim_value;
    char dim_param[MAX_TOKEN_LENGTH];
    bool is_param;
    struct DimNode* next;
} DimNode;

typedef struct TensorType {
    DataType elem_type;
    DimNode* dims;
    int num_dims;
} TensorType;

typedef struct AttributeValue {
    ValueType value_type;
    union {
        int int_val;
        float float_val;
        char* string_val;
        bool bool_val;
    } val;
} AttributeValue;

typedef struct treeNode TreeNode;

typedef struct SymbolEntry {
    char name[MAX_TOKEN_LENGTH];
    DataType data_type;
    int scope_level;
    bool is_defined;
    bool is_used;
    int line_defined;
    int line_first_used;
    TensorType* tensor_info;
    AttributeValue* attr_value;
    struct SymbolEntry* next_in_scope;
    struct SymbolEntry* next_in_bucket;
} SymbolEntry;

typedef struct ScopeNode {
    int level;
    SymbolEntry* symbols;
    struct ScopeNode* parent;
    struct ScopeNode* child;
    struct ScopeNode* sibling;
} ScopeNode;

typedef struct {
    char token_string[MAX_TOKEN_LENGTH];
    TokenType token;
    int line_number;
    int column;
} TokenInfo;

typedef struct HashBucket {
    SymbolEntry* entry;
    struct HashBucket* next;
} HashBucket;

typedef struct {
    HashBucket* buckets[HASH_TABLE_SIZE];
    ScopeNode* current_scope;
    int scope_count;
    int symbol_count;
} SymbolTable;

typedef struct {
    TreeNode* root;
    SymbolTable* symbol_table;
    TACInstruction* tac_first;
    TACInstruction* tac_last;
    ErrorInfo* error_list;
    int tac_count;
    int error_count;
    bool lexical_error;
    bool syntax_error;
    bool semantic_error;
} ASTContext;

extern FILE* source;
extern FILE* listing;
extern FILE* code_output;
extern FILE* tac_output;

extern int EchoSource;
extern int TraceScan;
extern int TraceParse;
extern int TraceAnalyze;
extern int TraceCode;
extern int Error;

extern int lineno;
extern int columnno;
extern char current_token_string[MAX_TOKEN_LENGTH];

void printToken(TokenType token, const char* token_string);
const char* tokenToString(TokenType token);
const char* nodeKindToString(NodeKind kind);
const char* dataTypeToString(DataType type);
const char* tacOpToString(TACOp op);
const char* errorTypeToString(ErrorType type);

#endif
