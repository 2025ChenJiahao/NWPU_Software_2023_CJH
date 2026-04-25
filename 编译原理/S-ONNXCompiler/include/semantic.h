#ifndef _SEMANTIC_H_
#define _SEMANTIC_H_

#include "globals.h"
#include "symboltable.h"
#include "parser.h"

typedef enum {
    SEMANTIC_OK,
    SEMANTIC_ERROR_NAME_CONFLICT,
    SEMANTIC_ERROR_UNDEFINED_USE,
    SEMANTIC_ERROR_TYPE_MISMATCH,
    SEMANTIC_ERROR_DIMENSION_MISMATCH,
    SEMANTIC_ERROR_DUPLICATE_DEFINITION
} SemanticResult;

SemanticResult analyzeSemantic(TreeNode* root, SymbolTable* symbol_table);
SemanticResult analyzeNode(TreeNode* node, SymbolTable* symbol_table);
SemanticResult checkNameConflict(SymbolTable* table, const char* name, int line);
SemanticResult checkUndefinedUse(SymbolTable* table, const char* name, int line);
SemanticResult checkTypeMismatch(DataType expected, DataType actual, int line);
SemanticResult checkTensorTypeConsistency(TensorType* expected, TensorType* actual, int line);
SemanticResult analyzeIrNode(TreeNode* node, SymbolTable* symbol_table);
SemanticResult analyzeAttributeNode(TreeNode* node, SymbolTable* symbol_table);
SemanticResult analyzeTensorNode(TreeNode* node, SymbolTable* symbol_table);
SemanticResult analyzeInputNode(TreeNode* node, SymbolTable* symbol_table);
SemanticResult analyzeOutputNode(TreeNode* node, SymbolTable* symbol_table);
SemanticResult analyzeInitializerNode(TreeNode* node, SymbolTable* symbol_table);
SemanticResult analyzeValueInfoNode(TreeNode* node, SymbolTable* symbol_table);
SemanticResult analyzeOpsetImportNode(TreeNode* node, SymbolTable* symbol_table);
void printSemanticAnalysisResult(SemanticResult result, int line);
void printSemanticError(const char* error_msg, int line);
const char* semanticResultToString(SemanticResult result);
void traverseASTForSemanticAnalysis(TreeNode* root, SymbolTable* symbol_table);
void collectUndefinedSymbols(SymbolTable* table);
void collectUnusedSymbols(SymbolTable* table);
void printSymbolAnalysisReport(SymbolTable* table);

#endif
