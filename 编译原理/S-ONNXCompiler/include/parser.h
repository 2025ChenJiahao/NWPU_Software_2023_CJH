#ifndef _PARSER_H_
#define _PARSER_H_

#include "globals.h"
#include "symboltable.h"

typedef struct treeNode {
    NodeKind node_kind;
    int line_number;
    int column;
    struct treeNode* child[MAX_CHILDREN];
    struct treeNode* sibling;
    struct treeNode* parent;
    int num_children;
    char name[MAX_TOKEN_LENGTH];
    char value[MAX_TOKEN_LENGTH];
    DataType data_type;
    TokenType token_type;
    union {
        struct {
            char op[MAX_TOKEN_LENGTH];
            char result[MAX_TOKEN_LENGTH];
        } expression;
        struct {
            DataType elem_type;
            DimNode* dims;
            int num_dims;
        } tensor_type;
        struct {
            char domain[MAX_TOKEN_LENGTH];
            int version;
        } opset_info;
        struct {
            char name[MAX_TOKEN_LENGTH];
            char type_str[MAX_TOKEN_LENGTH];
            DimNode* dims;
        } value_info;
        struct {
            char data_type[MAX_TOKEN_LENGTH];
            DimNode* dims;
            char raw_data[MAX_TOKEN_LENGTH];
        } tensor_def;
        struct {
            int int_val;
        } int_val;
        struct {
            float float_val;
        } float_val;
        struct {
            bool bool_val;
        } bool_val;
    } attr;
} TreeNode;

TreeNode* createNode(NodeKind kind);
void addChild(TreeNode* parent, TreeNode* child);
void addSibling(TreeNode* node, TreeNode* sibling);
void printTree(TreeNode* node);
void printTreeWithIndent(TreeNode* node, int indent);
void freeTree(TreeNode* node);
TreeNode* getChild(TreeNode* node, int index);
int getChildCount(TreeNode* node);

TreeNode* parse(FILE* file);
TreeNode* parseSOnnxModel(void);
TreeNode* parseIrDef(void);
TreeNode* parseProducerDef(void);
TreeNode* parseDocStringDef(void);
TreeNode* parseModelVersionDef(void);
TreeNode* parseInputDef(void);
TreeNode* parseOutputDef(void);
TreeNode* parseInitializerDef(void);
TreeNode* parseValueInfoDef(void);
TreeNode* parseAttributeRepeats(void);
TreeNode* parseAttributeDef(void);
TreeNode* parseValueDef(void);
TreeNode* parseValueInfoListDef(void);
TreeNode* parseTypeDef(void);
TreeNode* parseTensorTypeDef(void);
TreeNode* parseElemTypeDef(void);
TreeNode* parseShapeDef(void);
TreeNode* parseDimList(void);
TreeNode* parseDimRepeats(void);
TreeNode* parseDimDef(void);
TreeNode* parseTensorDef(void);
TreeNode* parseDataTypeDef(void);
TreeNode* parseDimsDef(void);
TreeNode* parseRawDataDef(void);
TreeNode* parseOpsetImportDef(void);
TreeNode* parseDomainDef(void);
TreeNode* parseVersionDef(void);
TreeNode* parseIdentifier(void);
TreeNode* parseInteger(void);
TreeNode* parseFloatNumber(void);
TreeNode* parseStringLiteral(void);
TreeNode* parseBoolLiteral(void);
TreeNode* parseBytesLiteral(void);
TreeNode* parseExpression(void);

TokenType getCurrentToken(void);
const char* getCurrentTokenString(void);
void advanceToken(void);
void expectToken(TokenType token);
bool checkToken(TokenType token);
void matchToken(TokenType token);
void matchTokenWithRecovery(TokenType token, const char* error_message);
void synchronizeParser(void);
void initParser(FILE* source_file);
void setTraceParse(bool trace);
void printParseTree(TreeNode* root);
ASTContext* getASTContext(void);
void setASTContext(ASTContext* context);
void reportSyntaxError(const char* format, ...);
void reportSyntaxErrorAt(int line, int col, const char* format, ...);

#endif
