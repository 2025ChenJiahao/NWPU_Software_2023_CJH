#include "semantic.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool trace_analyze = FALSE;

void setTraceAnalyze(bool trace) {
    trace_analyze = trace;
}

const char* semanticResultToString(SemanticResult result) {
    switch (result) {
        case SEMANTIC_OK:
            return "OK";
        case SEMANTIC_ERROR_NAME_CONFLICT:
            return "Name Conflict";
        case SEMANTIC_ERROR_UNDEFINED_USE:
            return "Undefined Use";
        case SEMANTIC_ERROR_TYPE_MISMATCH:
            return "Type Mismatch";
        case SEMANTIC_ERROR_DIMENSION_MISMATCH:
            return "Dimension Mismatch";
        case SEMANTIC_ERROR_DUPLICATE_DEFINITION:
            return "Duplicate Definition";
        default:
            return "Unknown Error";
    }
}

void printSemanticAnalysisResult(SemanticResult result, int line) {
    if (listing == NULL) {
        return;
    }

    if (result == SEMANTIC_OK) {
        if (trace_analyze) {
            fprintf(listing, "[SEMANTIC] Line %d: Analysis passed\n", line);
        }
    } else {
        fprintf(listing, "[SEMANTIC ERROR] Line %d: %s\n", line, semanticResultToString(result));
    }
}

void printSemanticError(const char* error_msg, int line) {
    if (listing == NULL) {
        return;
    }

    fprintf(listing, "[SEMANTIC ERROR] Line %d: %s\n", line, error_msg);
}

SemanticResult checkNameConflict(SymbolTable* table, const char* name, int line) {
    if (table == NULL || name == NULL) {
        return SEMANTIC_ERROR_NAME_CONFLICT;
    }

    SymbolEntry* existing = lookupSymbolInScope(table, name, getCurrentScopeLevel(table));
    if (existing != NULL) {
        reportSemanticError(line, 0, "Name '%s' conflicts with existing symbol at line %d", name, existing->line_defined);
        return SEMANTIC_ERROR_NAME_CONFLICT;
    }

    return SEMANTIC_OK;
}

SemanticResult checkUndefinedUse(SymbolTable* table, const char* name, int line) {
    if (table == NULL || name == NULL) {
        return SEMANTIC_ERROR_UNDEFINED_USE;
    }

    SymbolEntry* entry = lookupSymbol(table, name);
    if (entry == NULL) {
        reportSemanticError(line, 0, "Symbol '%s' is used but never defined", name);
        return SEMANTIC_ERROR_UNDEFINED_USE;
    }

    if (!entry->is_defined) {
        reportSemanticError(line, 0, "Symbol '%s' is used but not defined at this point", name);
        return SEMANTIC_ERROR_UNDEFINED_USE;
    }

    return SEMANTIC_OK;
}

SemanticResult checkTypeMismatch(DataType expected, DataType actual, int line) {
    if (expected != actual) {
        reportSemanticError(line, 0, "Type mismatch: expected %s but got %s",
                          dataTypeToString(expected), dataTypeToString(actual));
        return SEMANTIC_ERROR_TYPE_MISMATCH;
    }
    return SEMANTIC_OK;
}

SemanticResult checkTensorTypeConsistency(TensorType* expected, TensorType* actual, int line) {
    if (expected == NULL || actual == NULL) {
        return SEMANTIC_OK;
    }

    if (expected->elem_type != actual->elem_type) {
        reportSemanticError(line, 0, "Tensor element type mismatch: expected %s but got %s",
                          dataTypeToString(expected->elem_type), dataTypeToString(actual->elem_type));
        return SEMANTIC_ERROR_TYPE_MISMATCH;
    }

    if (expected->num_dims != actual->num_dims) {
        reportSemanticError(line, 0, "Tensor dimension count mismatch: expected %d but got %d",
                          expected->num_dims, actual->num_dims);
        return SEMANTIC_ERROR_DIMENSION_MISMATCH;
    }

    DimNode* expected_dim = expected->dims;
    DimNode* actual_dim = actual->dims;

    while (expected_dim != NULL && actual_dim != NULL) {
        if (expected_dim->is_param || actual_dim->is_param) {
            expected_dim = expected_dim->next;
            actual_dim = actual_dim->next;
            continue;
        }

        if (expected_dim->dim_value != actual_dim->dim_value) {
            reportSemanticError(line, 0, "Tensor dimension value mismatch: expected %d but got %d",
                              expected_dim->dim_value, actual_dim->dim_value);
            return SEMANTIC_ERROR_DIMENSION_MISMATCH;
        }

        expected_dim = expected_dim->next;
        actual_dim = actual_dim->next;
    }

    return SEMANTIC_OK;
}

SemanticResult analyzeIrNode(TreeNode* node, SymbolTable* symbol_table) {
    if (node == NULL || symbol_table == NULL) {
        return SEMANTIC_OK;
    }

    for (int i = 0; i < node->num_children; i++) {
        SemanticResult result = analyzeNode(node->child[i], symbol_table);
        if (result != SEMANTIC_OK) {
            return result;
        }
    }

    return SEMANTIC_OK;
}

SemanticResult analyzeAttributeNode(TreeNode* node, SymbolTable* symbol_table) {
    if (node == NULL || symbol_table == NULL) {
        return SEMANTIC_OK;
    }

    for (int i = 0; i < node->num_children; i++) {
        TreeNode* child = node->child[i];

        if (child->node_kind == NODE_IDENTIFIER) {
            char* attr_name = child->value;
            if (strlen(attr_name) == 0) {
                continue;
            }

            SemanticResult result = checkNameConflict(symbol_table, attr_name, child->line_number);
            if (result != SEMANTIC_OK) {
                return result;
            }

            insertSymbol(symbol_table, attr_name, TYPE_UNKNOWN, child->line_number);
            markSymbolAsDefined(symbol_table, attr_name);
        } else if (child->node_kind == NODE_VALUE_DEF) {
            if (child->num_children > 0) {
                TreeNode* name_child = NULL;
                for (int j = 0; j < node->num_children; j++) {
                    if (node->child[j]->node_kind == NODE_IDENTIFIER) {
                        name_child = node->child[j];
                        break;
                    }
                }

                if (name_child != NULL) {
                    DataType value_type = TYPE_UNKNOWN;
                    switch (child->data_type) {
                        case TYPE_INT:
                            value_type = TYPE_INT;
                            break;
                        case TYPE_FLOAT:
                            value_type = TYPE_FLOAT;
                            break;
                        case TYPE_STRING:
                            value_type = TYPE_STRING;
                            break;
                        case TYPE_BOOL:
                            value_type = TYPE_BOOL;
                            break;
                        default:
                            break;
                    }

                    if (value_type != TYPE_UNKNOWN) {
                        setSymbolType(symbol_table, name_child->value, value_type);
                    }
                }
            }
        }
    }

    return SEMANTIC_OK;
}

SemanticResult analyzeTensorNode(TreeNode* node, SymbolTable* symbol_table) {
    if (node == NULL || symbol_table == NULL) {
        return SEMANTIC_OK;
    }

    char tensor_name[MAX_TOKEN_LENGTH] = {0};
    TensorType* tensor_type = NULL;

    for (int i = 0; i < node->num_children; i++) {
        TreeNode* child = node->child[i];

        if (child->node_kind == NODE_IDENTIFIER) {
            strncpy(tensor_name, child->value, MAX_TOKEN_LENGTH - 1);
        } else if (child->node_kind == NODE_DATA_TYPE_DEF) {
            tensor_type = createTensorType(child->data_type);
        } else if (child->node_kind == NODE_DIMS_DEF) {
            if (tensor_type != NULL) {
                for (int j = 0; j < child->num_children; j++) {
                    TreeNode* dim = child->child[j];
                    if (dim->node_kind == NODE_INTEGER) {
                        int dim_val = atoi(dim->value);
                        addDimensionToTensor(tensor_type, dim_val, NULL);
                    }
                }
            }
        }
    }

    if (tensor_name[0] != '\0') {
        SemanticResult result = checkNameConflict(symbol_table, tensor_name, node->line_number);
        if (result != SEMANTIC_OK) {
            return result;
        }

        SymbolEntry* entry = insertSymbol(symbol_table, tensor_name, TYPE_TENSOR, node->line_number);
        if (entry != NULL && tensor_type != NULL) {
            setSymbolTensorInfo(symbol_table, tensor_name, tensor_type);
            markSymbolAsDefined(symbol_table, tensor_name);
        }
    }

    return SEMANTIC_OK;
}

SemanticResult analyzeInputNode(TreeNode* node, SymbolTable* symbol_table) {
    if (node == NULL || symbol_table == NULL) {
        return SEMANTIC_OK;
    }

    char input_name[MAX_TOKEN_LENGTH] = {0};
    DataType input_type = TYPE_UNKNOWN;

    for (int i = 0; i < node->num_children; i++) {
        TreeNode* child = node->child[i];

        if (child->node_kind == NODE_IDENTIFIER) {
            strncpy(input_name, child->value, MAX_TOKEN_LENGTH - 1);
        } else if (child->node_kind == NODE_TYPE_DEF) {
            if (child->num_children > 0) {
                TreeNode* tensor_type_node = child->child[0];
                if (tensor_type_node != NULL && tensor_type_node->num_children > 0) {
                    for (int j = 0; j < tensor_type_node->num_children; j++) {
                        TreeNode* type_child = tensor_type_node->child[j];
                        if (type_child->node_kind == NODE_ELEM_TYPE_DEF) {
                            input_type = type_child->data_type;
                        }
                    }
                }
            }
        }
    }

    if (input_name[0] != '\0') {
        SemanticResult result = checkNameConflict(symbol_table, input_name, node->line_number);
        if (result != SEMANTIC_OK) {
            return result;
        }

        SymbolEntry* entry = insertSymbol(symbol_table, input_name, TYPE_TENSOR, node->line_number);
        if (entry != NULL) {
            markSymbolAsDefined(symbol_table, input_name);
        }
    }

    return SEMANTIC_OK;
}

SemanticResult analyzeOutputNode(TreeNode* node, SymbolTable* symbol_table) {
    if (node == NULL || symbol_table == NULL) {
        return SEMANTIC_OK;
    }

    char output_name[MAX_TOKEN_LENGTH] = {0};

    for (int i = 0; i < node->num_children; i++) {
        TreeNode* child = node->child[i];

        if (child->node_kind == NODE_IDENTIFIER) {
            strncpy(output_name, child->value, MAX_TOKEN_LENGTH - 1);
        }
    }

    if (output_name[0] != '\0') {
        SemanticResult result = checkUndefinedUse(symbol_table, output_name, node->line_number);
        if (result != SEMANTIC_OK) {
            return result;
        }

        markSymbolAsUsed(symbol_table, output_name);
    }

    return SEMANTIC_OK;
}

SemanticResult analyzeInitializerNode(TreeNode* node, SymbolTable* symbol_table) {
    if (node == NULL || symbol_table == NULL) {
        return SEMANTIC_OK;
    }

    char initializer_name[MAX_TOKEN_LENGTH] = {0};
    TensorType* tensor_type = NULL;

    for (int i = 0; i < node->num_children; i++) {
        TreeNode* child = node->child[i];

        if (child->node_kind == NODE_IDENTIFIER) {
            strncpy(initializer_name, child->value, MAX_TOKEN_LENGTH - 1);
        } else if (child->node_kind == NODE_TYPE_DEF) {
            if (child->num_children > 0) {
                TreeNode* tensor_type_node = child->child[0];
                if (tensor_type_node != NULL) {
                    tensor_type = createTensorType(TYPE_UNKNOWN);

                    for (int j = 0; j < tensor_type_node->num_children; j++) {
                        TreeNode* type_child = tensor_type_node->child[j];
                        if (type_child->node_kind == NODE_ELEM_TYPE_DEF) {
                            tensor_type->elem_type = type_child->data_type;
                        } else if (type_child->node_kind == NODE_SHAPE_DEF) {
                            for (int k = 0; k < type_child->num_children; k++) {
                                TreeNode* dim_repeats = type_child->child[k];
                                if (dim_repeats->node_kind == NODE_DIM_REPEATS) {
                                    for (int m = 0; m < dim_repeats->num_children; m++) {
                                        TreeNode* dim = dim_repeats->child[m];
                                        if (dim->node_kind == NODE_DIM_DEF) {
                                            if (dim->data_type == TYPE_INT) {
                                                addDimensionToTensor(tensor_type, atoi(dim->value), NULL);
                                            } else if (dim->data_type == TYPE_STRING) {
                                                addDimensionToTensor(tensor_type, 0, dim->value);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (initializer_name[0] != '\0') {
        SemanticResult result = checkNameConflict(symbol_table, initializer_name, node->line_number);
        if (result != SEMANTIC_OK) {
            return result;
        }

        SymbolEntry* entry = insertSymbol(symbol_table, initializer_name, TYPE_TENSOR, node->line_number);
        if (entry != NULL) {
            if (tensor_type != NULL) {
                setSymbolTensorInfo(symbol_table, initializer_name, tensor_type);
            }
            markSymbolAsDefined(symbol_table, initializer_name);
        }
    }

    return SEMANTIC_OK;
}

SemanticResult analyzeValueInfoNode(TreeNode* node, SymbolTable* symbol_table) {
    if (node == NULL || symbol_table == NULL) {
        return SEMANTIC_OK;
    }

    char value_name[MAX_TOKEN_LENGTH] = {0};

    for (int i = 0; i < node->num_children; i++) {
        TreeNode* child = node->child[i];

        if (child->node_kind == NODE_IDENTIFIER) {
            strncpy(value_name, child->value, MAX_TOKEN_LENGTH - 1);
        }
    }

    if (value_name[0] != '\0') {
        SemanticResult result = checkNameConflict(symbol_table, value_name, node->line_number);
        if (result != SEMANTIC_OK) {
            return result;
        }

        insertSymbol(symbol_table, value_name, TYPE_TENSOR, node->line_number);
        markSymbolAsDefined(symbol_table, value_name);
    }

    return SEMANTIC_OK;
}

SemanticResult analyzeOpsetImportNode(TreeNode* node, SymbolTable* symbol_table) {
    if (node == NULL || symbol_table == NULL) {
        return SEMANTIC_OK;
    }

    char domain[MAX_TOKEN_LENGTH] = {0};
    int version = 0;

    for (int i = 0; i < node->num_children; i++) {
        TreeNode* child = node->child[i];

        if (child->node_kind == NODE_DOMAIN_DEF) {
            strncpy(domain, child->value, MAX_TOKEN_LENGTH - 1);
        } else if (child->node_kind == NODE_VERSION_DEF) {
            version = atoi(child->value);
        }
    }

    if (domain[0] != '\0') {
        SemanticResult result = checkNameConflict(symbol_table, domain, node->line_number);
        if (result != SEMANTIC_OK) {
            return result;
        }

        SymbolEntry* entry = insertSymbol(symbol_table, domain, TYPE_OPSET_IMPORT, node->line_number);
        if (entry != NULL) {
            markSymbolAsDefined(symbol_table, domain);
        }
    }

    return SEMANTIC_OK;
}

SemanticResult analyzeNode(TreeNode* node, SymbolTable* symbol_table) {
    if (node == NULL) {
        return SEMANTIC_OK;
    }

    SemanticResult result = SEMANTIC_OK;

    switch (node->node_kind) {
        case NODE_ROOT:
        case NODE_IR:
            result = analyzeIrNode(node, symbol_table);
            break;

        case NODE_ATTRIBUTE_REPEATS:
            for (int i = 0; i < node->num_children; i++) {
                result = analyzeAttributeNode(node->child[i], symbol_table);
                if (result != SEMANTIC_OK) {
                    return result;
                }
            }
            break;

        case NODE_ATTRIBUTE_DEF:
            result = analyzeAttributeNode(node, symbol_table);
            break;

        case NODE_TENSOR_DEF:
            result = analyzeTensorNode(node, symbol_table);
            break;

        case NODE_INPUT_DEF:
            result = analyzeInputNode(node, symbol_table);
            break;

        case NODE_OUTPUT_DEF:
            result = analyzeOutputNode(node, symbol_table);
            break;

        case NODE_INITIALIZER_DEF:
            result = analyzeInitializerNode(node, symbol_table);
            break;

        case NODE_VALUE_INFO_DEF:
            result = analyzeValueInfoNode(node, symbol_table);
            break;

        case NODE_OPSET_IMPORT_DEF:
            result = analyzeOpsetImportNode(node, symbol_table);
            break;

        case NODE_IDENTIFIER:
            if (node->value[0] != '\0') {
                result = checkUndefinedUse(symbol_table, node->value, node->line_number);
                if (result == SEMANTIC_OK) {
                    markSymbolAsUsed(symbol_table, node->value);
                }
            }
            break;

        default:
            for (int i = 0; i < node->num_children; i++) {
                result = analyzeNode(node->child[i], symbol_table);
                if (result != SEMANTIC_OK) {
                    return result;
                }
            }
            break;
    }

    return result;
}

void traverseASTForSemanticAnalysis(TreeNode* root, SymbolTable* symbol_table) {
    if (root == NULL || symbol_table == NULL) {
        return;
    }

    TreeNode* current = root;
    while (current != NULL) {
        SemanticResult result = analyzeNode(current, symbol_table);
        if (result != SEMANTIC_OK) {
            printSemanticAnalysisResult(result, current->line_number);
        }

        for (int i = 0; i < current->num_children; i++) {
            traverseASTForSemanticAnalysis(current->child[i], symbol_table);
        }

        current = current->sibling;
    }
}

SemanticResult analyzeSemantic(TreeNode* root, SymbolTable* symbol_table) {
    if (root == NULL || symbol_table == NULL) {
        return SEMANTIC_ERROR_UNDEFINED_USE;
    }

    if (listing != NULL) {
        fprintf(listing, "\n============= SEMANTIC ANALYSIS =============\n");
    }

    enterScope(symbol_table);

    traverseASTForSemanticAnalysis(root, symbol_table);

    exitScope(symbol_table);

    collectUndefinedSymbols(symbol_table);
    collectUnusedSymbols(symbol_table);

    if (listing != NULL) {
        printSymbolTable(symbol_table);
        fprintf(listing, "==============================================\n\n");
    }

    return SEMANTIC_OK;
}

void collectUndefinedSymbols(SymbolTable* table) {
    if (table == NULL || listing == NULL) {
        return;
    }

    fprintf(listing, "\n--- Checking for undefined symbol uses ---\n");

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashBucket* bucket = table->buckets[i];
        while (bucket != NULL && bucket->entry != NULL) {
            if (!bucket->entry->is_defined) {
                reportSemanticError(bucket->entry->line_defined, 0,
                                  "Symbol '%s' is defined but never used",
                                  bucket->entry->name);
            }
            bucket = bucket->next;
        }
    }
}

void collectUnusedSymbols(SymbolTable* table) {
    if (table == NULL || listing == NULL) {
        return;
    }

    fprintf(listing, "\n--- Checking for unused symbols ---\n");

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashBucket* bucket = table->buckets[i];
        while (bucket != NULL && bucket->entry != NULL) {
            if (!bucket->entry->is_used) {
                reportSemanticError(bucket->entry->line_defined, 0,
                                  "Symbol '%s' is defined but never referenced",
                                  bucket->entry->name);
            }
            bucket = bucket->next;
        }
    }
}

void printSymbolAnalysisReport(SymbolTable* table) {
    if (table == NULL || listing == NULL) {
        return;
    }

    fprintf(listing, "\n============= SYMBOL ANALYSIS REPORT =============\n");

    int defined_count = 0;
    int used_count = 0;
    int undefined_count = 0;
    int unused_count = 0;

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashBucket* bucket = table->buckets[i];
        while (bucket != NULL && bucket->entry != NULL) {
            if (bucket->entry->is_defined) {
                defined_count++;
            }
            if (bucket->entry->is_used) {
                used_count++;
            }
            if (!bucket->entry->is_defined && bucket->entry->is_used) {
                undefined_count++;
            }
            if (bucket->entry->is_defined && !bucket->entry->is_used) {
                unused_count++;
            }
            bucket = bucket->next;
        }
    }

    fprintf(listing, "Total Defined Symbols: %d\n", defined_count);
    fprintf(listing, "Total Used Symbols: %d\n", used_count);
    fprintf(listing, "Undefined but Used: %d\n", undefined_count);
    fprintf(listing, "Defined but Unused: %d\n", unused_count);
    fprintf(listing, "====================================================\n\n");
}
