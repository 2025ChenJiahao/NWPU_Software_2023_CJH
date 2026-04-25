#ifndef _SYMBOLTABLE_H_
#define _SYMBOLTABLE_H_

#include "globals.h"

SymbolTable* createSymbolTable(void);
void destroySymbolTable(SymbolTable* table);
SymbolEntry* insertSymbol(SymbolTable* table, const char* name, DataType type, int line);
SymbolEntry* lookupSymbol(SymbolTable* table, const char* name);
SymbolEntry* lookupSymbolInScope(SymbolTable* table, const char* name, int scope_level);
bool insertSymbolIfNotExists(SymbolTable* table, const char* name, DataType type, int line);
void enterScope(SymbolTable* table);
void exitScope(SymbolTable* table);
int getCurrentScopeLevel(SymbolTable* table);
void printSymbolTable(SymbolTable* table);
void printSymbolEntry(SymbolEntry* entry);
void markSymbolAsDefined(SymbolTable* table, const char* name);
void markSymbolAsUsed(SymbolTable* table, const char* name);
SymbolEntry* getAllSymbolsInScope(SymbolTable* table, int scope_level);
int getSymbolCount(SymbolTable* table);
void printSymbolTableDetailed(SymbolTable* table);
bool checkSymbolDefined(SymbolTable* table, const char* name);
bool checkSymbolUsed(SymbolTable* table, const char* name);
void setSymbolType(SymbolTable* table, const char* name, DataType type);
void setSymbolTensorInfo(SymbolTable* table, const char* name, TensorType* tensor_info);
void setSymbolAttributeValue(SymbolTable* table, const char* name, AttributeValue* attr_value);
TensorType* createTensorType(DataType elem_type);
void addDimensionToTensor(TensorType* tensor, int dim_value, const char* dim_param);
void printTensorType(TensorType* tensor);
void freeTensorType(TensorType* tensor);
SymbolEntry* getNextSymbolInScope(SymbolTable* table, SymbolEntry* current);
SymbolEntry* getNextSymbolInBucket(SymbolEntry* current);

#endif
