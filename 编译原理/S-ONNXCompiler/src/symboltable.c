#include "symboltable.h"
#include "globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int hash(const char* str) {
    unsigned int hash_value = 5381;
    int c;

    while ((c = *str++)) {
        hash_value = ((hash_value << 5) + hash_value) + c;
    }

    return hash_value % HASH_TABLE_SIZE;
}

SymbolTable* createSymbolTable(void) {
    SymbolTable* table = (SymbolTable*)malloc(sizeof(SymbolTable));
    if (table == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for symbol table\n");
        return NULL;
    }

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        table->buckets[i] = NULL;
    }

    table->current_scope = (ScopeNode*)malloc(sizeof(ScopeNode));
    if (table->current_scope == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for scope node\n");
        free(table);
        return NULL;
    }

    table->current_scope->level = 0;
    table->current_scope->symbols = NULL;
    table->current_scope->parent = NULL;
    table->current_scope->child = NULL;
    table->current_scope->sibling = NULL;

    table->scope_count = 1;
    table->symbol_count = 0;

    return table;
}

void destroySymbolTable(SymbolTable* table) {
    if (table == NULL) {
        return;
    }

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashBucket* bucket = table->buckets[i];
        while (bucket != NULL) {
            HashBucket* next = bucket->next;
            if (bucket->entry != NULL) {
                if (bucket->entry->tensor_info != NULL) {
                    freeTensorType(bucket->entry->tensor_info);
                }
                free(bucket->entry);
            }
            free(bucket);
            bucket = next;
        }
    }

    ScopeNode* current_scope = table->current_scope;
    while (current_scope != NULL) {
        ScopeNode* parent = current_scope->parent;
        free(current_scope);
        current_scope = parent;
    }

    free(table);
}

SymbolEntry* insertSymbol(SymbolTable* table, const char* name, DataType type, int line) {
    if (table == NULL || name == NULL) {
        return NULL;
    }

    unsigned int index = hash(name);

    HashBucket* new_bucket = (HashBucket*)malloc(sizeof(HashBucket));
    if (new_bucket == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for hash bucket\n");
        return NULL;
    }

    SymbolEntry* new_entry = (SymbolEntry*)malloc(sizeof(SymbolEntry));
    if (new_entry == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for symbol entry\n");
        free(new_bucket);
        return NULL;
    }

    strncpy(new_entry->name, name, MAX_TOKEN_LENGTH - 1);
    new_entry->name[MAX_TOKEN_LENGTH - 1] = '\0';
    new_entry->data_type = type;
    new_entry->scope_level = table->current_scope->level;
    new_entry->is_defined = FALSE;
    new_entry->is_used = FALSE;
    new_entry->line_defined = line;
    new_entry->line_first_used = -1;
    new_entry->tensor_info = NULL;
    new_entry->attr_value = NULL;
    new_entry->next_in_scope = NULL;
    new_entry->next_in_bucket = NULL;

    new_bucket->entry = new_entry;
    new_bucket->next = table->buckets[index];
    table->buckets[index] = new_bucket;

    new_entry->next_in_bucket = new_bucket->next ? new_bucket->next->entry : NULL;

    SymbolEntry* scope_entry = table->current_scope->symbols;
    if (scope_entry == NULL) {
        table->current_scope->symbols = new_entry;
    } else {
        while (scope_entry->next_in_scope != NULL) {
            scope_entry = scope_entry->next_in_scope;
        }
        scope_entry->next_in_scope = new_entry;
    }

    table->symbol_count++;

    return new_entry;
}

SymbolEntry* lookupSymbol(SymbolTable* table, const char* name) {
    if (table == NULL || name == NULL) {
        return NULL;
    }

    unsigned int index = hash(name);

    HashBucket* bucket = table->buckets[index];
    while (bucket != NULL) {
        if (bucket->entry != NULL && strcmp(bucket->entry->name, name) == 0) {
            return bucket->entry;
        }
        bucket = bucket->next;
    }

    return NULL;
}

SymbolEntry* lookupSymbolInScope(SymbolTable* table, const char* name, int scope_level) {
    if (table == NULL || name == NULL) {
        return NULL;
    }

    unsigned int index = hash(name);

    HashBucket* bucket = table->buckets[index];
    while (bucket != NULL) {
        if (bucket->entry != NULL &&
            strcmp(bucket->entry->name, name) == 0 &&
            bucket->entry->scope_level == scope_level) {
            return bucket->entry;
        }
        bucket = bucket->next;
    }

    return NULL;
}

bool insertSymbolIfNotExists(SymbolTable* table, const char* name, DataType type, int line) {
    if (table == NULL || name == NULL) {
        return FALSE;
    }

    SymbolEntry* existing = lookupSymbolInScope(table, name, table->current_scope->level);
    if (existing != NULL) {
        return FALSE;
    }

    return insertSymbol(table, name, type, line) != NULL;
}

void enterScope(SymbolTable* table) {
    if (table == NULL) {
        return;
    }

    ScopeNode* new_scope = (ScopeNode*)malloc(sizeof(ScopeNode));
    if (new_scope == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for new scope\n");
        return;
    }

    new_scope->level = table->scope_count;
    new_scope->symbols = NULL;
    new_scope->parent = table->current_scope;
    new_scope->child = NULL;
    new_scope->sibling = NULL;

    if (table->current_scope->child == NULL) {
        table->current_scope->child = new_scope;
    } else {
        ScopeNode* sibling = table->current_scope->child;
        while (sibling->sibling != NULL) {
            sibling = sibling->sibling;
        }
        sibling->sibling = new_scope;
    }

    table->current_scope = new_scope;
    table->scope_count++;
}

void exitScope(SymbolTable* table) {
    if (table == NULL || table->current_scope->parent == NULL) {
        return;
    }

    table->current_scope = table->current_scope->parent;
}

int getCurrentScopeLevel(SymbolTable* table) {
    if (table == NULL) {
        return -1;
    }
    return table->current_scope->level;
}

void printSymbolTable(SymbolTable* table) {
    if (table == NULL || listing == NULL) {
        return;
    }

    fprintf(listing, "\n============= SYMBOL TABLE =============\n");
    fprintf(listing, "%-30s %-15s %-10s %-10s %-10s\n",
            "NAME", "TYPE", "SCOPE", "DEFINED", "USED");
    fprintf(listing, "-----------------------------------------------------------------------\n");

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashBucket* bucket = table->buckets[i];
        while (bucket != NULL && bucket->entry != NULL) {
            fprintf(listing, "%-30s %-15s %-10d %-10s %-10s\n",
                    bucket->entry->name,
                    dataTypeToString(bucket->entry->data_type),
                    bucket->entry->scope_level,
                    bucket->entry->is_defined ? "YES" : "NO",
                    bucket->entry->is_used ? "YES" : "NO");
            bucket = bucket->next;
        }
    }

    fprintf(listing, "-----------------------------------------------------------------------\n");
    fprintf(listing, "Total symbols: %d, Total scopes: %d\n", table->symbol_count, table->scope_count);
    fprintf(listing, "=======================================\n\n");
}

void printSymbolEntry(SymbolEntry* entry) {
    if (entry == NULL || listing == NULL) {
        return;
    }

    fprintf(listing, "Symbol: %s\n", entry->name);
    fprintf(listing, "  Type: %s\n", dataTypeToString(entry->data_type));
    fprintf(listing, "  Scope Level: %d\n", entry->scope_level);
    fprintf(listing, "  Defined: %s\n", entry->is_defined ? "YES" : "NO");
    fprintf(listing, "  Used: %s\n", entry->is_used ? "YES" : "NO");
    fprintf(listing, "  Line Defined: %d\n", entry->line_defined);
    fprintf(listing, "  Line First Used: %d\n", entry->line_first_used);

    if (entry->tensor_info != NULL) {
        fprintf(listing, "  Tensor Info:\n");
        fprintf(listing, "    Element Type: %s\n", dataTypeToString(entry->tensor_info->elem_type));
        fprintf(listing, "    Num Dims: %d\n", entry->tensor_info->num_dims);
    }
}

void markSymbolAsDefined(SymbolTable* table, const char* name) {
    if (table == NULL || name == NULL) {
        return;
    }

    SymbolEntry* entry = lookupSymbol(table, name);
    if (entry != NULL) {
        entry->is_defined = TRUE;
        entry->line_defined = getCurrentScopeLevel(table);
    }
}

void markSymbolAsUsed(SymbolTable* table, const char* name) {
    if (table == NULL || name == NULL) {
        return;
    }

    SymbolEntry* entry = lookupSymbol(table, name);
    if (entry != NULL) {
        entry->is_used = TRUE;
        if (entry->line_first_used == -1) {
            entry->line_first_used = getCurrentScopeLevel(table);
        }
    }
}

SymbolEntry* getAllSymbolsInScope(SymbolTable* table, int scope_level) {
    if (table == NULL) {
        return NULL;
    }

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashBucket* bucket = table->buckets[i];
        while (bucket != NULL && bucket->entry != NULL) {
            if (bucket->entry->scope_level == scope_level) {
                return bucket->entry;
            }
            bucket = bucket->next;
        }
    }

    return NULL;
}

int getSymbolCount(SymbolTable* table) {
    if (table == NULL) {
        return 0;
    }
    return table->symbol_count;
}

void printSymbolTableDetailed(SymbolTable* table) {
    if (table == NULL || listing == NULL) {
        return;
    }

    fprintf(listing, "\n============= SYMBOL TABLE (DETAILED) =============\n");

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashBucket* bucket = table->buckets[i];
        while (bucket != NULL && bucket->entry != NULL) {
            printSymbolEntry(bucket->entry);
            fprintf(listing, "\n");
            bucket = bucket->next;
        }
    }

    fprintf(listing, "====================================================\n\n");
}

bool checkSymbolDefined(SymbolTable* table, const char* name) {
    if (table == NULL || name == NULL) {
        return FALSE;
    }

    SymbolEntry* entry = lookupSymbol(table, name);
    return entry != NULL && entry->is_defined;
}

bool checkSymbolUsed(SymbolTable* table, const char* name) {
    if (table == NULL || name == NULL) {
        return FALSE;
    }

    SymbolEntry* entry = lookupSymbol(table, name);
    return entry != NULL && entry->is_used;
}

void setSymbolType(SymbolTable* table, const char* name, DataType type) {
    if (table == NULL || name == NULL) {
        return;
    }

    SymbolEntry* entry = lookupSymbol(table, name);
    if (entry != NULL) {
        entry->data_type = type;
    }
}

void setSymbolTensorInfo(SymbolTable* table, const char* name, TensorType* tensor_info) {
    if (table == NULL || name == NULL) {
        return;
    }

    SymbolEntry* entry = lookupSymbol(table, name);
    if (entry != NULL) {
        if (entry->tensor_info != NULL) {
            freeTensorType(entry->tensor_info);
        }
        entry->tensor_info = tensor_info;
    }
}

void setSymbolAttributeValue(SymbolTable* table, const char* name, AttributeValue* attr_value) {
    if (table == NULL || name == NULL) {
        return;
    }

    SymbolEntry* entry = lookupSymbol(table, name);
    if (entry != NULL) {
        entry->attr_value = attr_value;
    }
}

TensorType* createTensorType(DataType elem_type) {
    TensorType* tensor = (TensorType*)malloc(sizeof(TensorType));
    if (tensor == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for tensor type\n");
        return NULL;
    }

    tensor->elem_type = elem_type;
    tensor->dims = NULL;
    tensor->num_dims = 0;

    return tensor;
}

void addDimensionToTensor(TensorType* tensor, int dim_value, const char* dim_param) {
    if (tensor == NULL) {
        return;
    }

    DimNode* new_dim = (DimNode*)malloc(sizeof(DimNode));
    if (new_dim == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for dimension node\n");
        return;
    }

    new_dim->dim_value = dim_value;
    if (dim_param != NULL) {
        strncpy(new_dim->dim_param, dim_param, MAX_TOKEN_LENGTH - 1);
        new_dim->is_param = TRUE;
    } else {
        new_dim->dim_param[0] = '\0';
        new_dim->is_param = FALSE;
    }
    new_dim->next = NULL;

    if (tensor->dims == NULL) {
        tensor->dims = new_dim;
    } else {
        DimNode* current = tensor->dims;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_dim;
    }

    tensor->num_dims++;
}

void printTensorType(TensorType* tensor) {
    if (tensor == NULL || listing == NULL) {
        return;
    }

    fprintf(listing, "TensorType(elem_type=%s, num_dims=%d, dims=[",
            dataTypeToString(tensor->elem_type), tensor->num_dims);

    DimNode* current = tensor->dims;
    while (current != NULL) {
        if (current->is_param) {
            fprintf(listing, "%s", current->dim_param);
        } else {
            fprintf(listing, "%d", current->dim_value);
        }
        if (current->next != NULL) {
            fprintf(listing, ", ");
        }
        current = current->next;
    }

    fprintf(listing, "])");
}

void freeTensorType(TensorType* tensor) {
    if (tensor == NULL) {
        return;
    }

    DimNode* current = tensor->dims;
    while (current != NULL) {
        DimNode* next = current->next;
        free(current);
        current = next;
    }

    free(tensor);
}

SymbolEntry* getNextSymbolInScope(SymbolTable* table, SymbolEntry* current) {
    (void)table;
    if (current == NULL) {
        return NULL;
    }
    return current->next_in_scope;
}

SymbolEntry* getNextSymbolInBucket(SymbolEntry* current) {
    if (current == NULL) {
        return NULL;
    }
    return current->next_in_bucket;
}
