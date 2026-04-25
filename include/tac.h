#ifndef _TAC_H_
#define _TAC_H_

#include "globals.h"
#include "symboltable.h"
#include "parser.h"

typedef struct TACGenerator {
    TACInstruction* first;
    TACInstruction* last;
    int instruction_count;
    int temp_var_counter;
    int label_counter;
    bool trace_generation;
} TACGenerator;

TACGenerator* createTACGenerator(void);
void destroyTACGenerator(TACGenerator* generator);
TACInstruction* createTACInstruction(TACOp op, const char* result, const char* op1, const char* op2, const char* op3);
void emitTAC(TACGenerator* generator, TACInstruction* instruction);
void emitTACOp(TACGenerator* generator, TACOp op, const char* result, const char* op1, const char* op2, const char* op3);
void emitInputOp(TACGenerator* generator, const char* input_name);
void emitOutputOp(TACGenerator* generator, const char* output_name);
void emitInitializerOp(TACGenerator* generator, const char* tensor_name);
void emitAttributeOp(TACGenerator* generator, const char* attr_name, const char* attr_value);
void emitTensorOp(TACGenerator* generator, const char* tensor_name, const char* data_type);
void emitConstOp(TACGenerator* generator, const char* result, const char* value, DataType type);
void emitAssignOp(TACGenerator* generator, const char* result, const char* op1);
void emitLabel(TACGenerator* generator, const char* label);
void emitGoto(TACGenerator* generator, const char* label);
void emitIfGoto(TACGenerator* generator, const char* condition, const char* label);
void emitFunction(TACGenerator* generator, const char* func_name);
void emitReturn(TACGenerator* generator, const char* return_value);
void emitCall(TACGenerator* generator, const char* result, const char* func_name);
void emitParam(TACGenerator* generator, const char* param_name);
char* generateTempVar(TACGenerator* generator);
char* generateLabel(TACGenerator* generator);
void generateTACFromAST(TACGenerator* generator, TreeNode* root, SymbolTable* symbol_table);
void generateTACFromNode(TACGenerator* generator, TreeNode* node, SymbolTable* symbol_table);
void generateTACFromIrNode(TACGenerator* generator, TreeNode* node, SymbolTable* symbol_table);
void generateTACFromAttributeNode(TACGenerator* generator, TreeNode* node, SymbolTable* symbol_table);
void generateTACFromInputNode(TACGenerator* generator, TreeNode* node, SymbolTable* symbol_table);
void generateTACFromOutputNode(TACGenerator* generator, TreeNode* node, SymbolTable* symbol_table);
void generateTACFromInitializerNode(TACGenerator* generator, TreeNode* node, SymbolTable* symbol_table);
void generateTACFromTensorNode(TACGenerator* generator, TreeNode* node, SymbolTable* symbol_table);
void generateTACFromOpsetImportNode(TACGenerator* generator, TreeNode* node, SymbolTable* symbol_table);
void printTAC(TACGenerator* generator, FILE* output);
void printTACInstruction(TACInstruction* instr, FILE* output);
void setTraceTACGeneration(bool trace);
int getTACInstructionCount(TACGenerator* generator);
TACInstruction* getTACFirst(TACGenerator* generator);
TACInstruction* getTACLast(TACGenerator* generator);

#endif
