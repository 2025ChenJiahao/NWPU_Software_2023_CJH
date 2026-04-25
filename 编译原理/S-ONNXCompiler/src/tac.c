#include "tac.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool trace_generation = FALSE;

void setTraceTACGeneration(bool trace) {
    trace_generation = trace;
}

TACGenerator* createTACGenerator(void) {
    TACGenerator* generator = (TACGenerator*)malloc(sizeof(TACGenerator));
    if (generator == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for TAC generator\n");
        return NULL;
    }

    generator->first = NULL;
    generator->last = NULL;
    generator->instruction_count = 0;
    generator->temp_var_counter = 0;
    generator->label_counter = 0;
    generator->trace_generation = FALSE;

    return generator;
}

void destroyTACGenerator(TACGenerator* generator) {
    if (generator == NULL) {
        return;
    }

    TACInstruction* current = generator->first;
    while (current != NULL) {
        TACInstruction* next = current->next;
        free(current);
        current = next;
    }

    free(generator);
}

TACInstruction* createTACInstruction(TACOp op, const char* result, const char* op1, const char* op2, const char* op3) {
    TACInstruction* instr = (TACInstruction*)malloc(sizeof(TACInstruction));
    if (instr == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for TAC instruction\n");
        return NULL;
    }

    instr->label = -1;
    instr->op = op;
    instr->next = NULL;

    if (result != NULL) {
        strncpy(instr->result, result, MAX_TOKEN_LENGTH - 1);
        instr->result[MAX_TOKEN_LENGTH - 1] = '\0';
    } else {
        instr->result[0] = '\0';
    }

    if (op1 != NULL) {
        strncpy(instr->operand1, op1, MAX_TOKEN_LENGTH - 1);
        instr->operand1[MAX_TOKEN_LENGTH - 1] = '\0';
    } else {
        instr->operand1[0] = '\0';
    }

    if (op2 != NULL) {
        strncpy(instr->operand2, op2, MAX_TOKEN_LENGTH - 1);
        instr->operand2[MAX_TOKEN_LENGTH - 1] = '\0';
    } else {
        instr->operand2[0] = '\0';
    }

    if (op3 != NULL) {
        strncpy(instr->operand3, op3, MAX_TOKEN_LENGTH - 1);
        instr->operand3[MAX_TOKEN_LENGTH - 1] = '\0';
    } else {
        instr->operand3[0] = '\0';
    }

    instr->comment[0] = '\0';

    return instr;
}

void emitTAC(TACGenerator* generator, TACInstruction* instruction) {
    if (generator == NULL || instruction == NULL) {
        return;
    }

    instruction->label = generator->instruction_count++;

    if (generator->last != NULL) {
        generator->last->next = instruction;
    } else {
        generator->first = instruction;
    }

    generator->last = instruction;

    if (trace_generation && listing != NULL) {
        fprintf(listing, "  TAC[%3d]: ", instruction->label);
        printTACInstruction(instruction, listing);
    }
}

void emitTACOp(TACGenerator* generator, TACOp op, const char* result, const char* op1, const char* op2, const char* op3) {
    TACInstruction* instr = createTACInstruction(op, result, op1, op2, op3);
    emitTAC(generator, instr);
}

void emitInputOp(TACGenerator* generator, const char* input_name) {
    emitTACOp(generator, TAC_OP_INPUT, input_name, NULL, NULL, NULL);
}

void emitOutputOp(TACGenerator* generator, const char* output_name) {
    emitTACOp(generator, TAC_OP_OUTPUT, output_name, NULL, NULL, NULL);
}

void emitInitializerOp(TACGenerator* generator, const char* tensor_name) {
    emitTACOp(generator, TAC_OP_INITIALIZER, tensor_name, NULL, NULL, NULL);
}

void emitAttributeOp(TACGenerator* generator, const char* attr_name, const char* attr_value) {
    emitTACOp(generator, TAC_OP_ATTRIBUTE, attr_name, attr_value, NULL, NULL);
}

void emitTensorOp(TACGenerator* generator, const char* tensor_name, const char* data_type) {
    emitTACOp(generator, TAC_OP_TENSOR, tensor_name, data_type, NULL, NULL);
}

void emitConstOp(TACGenerator* generator, const char* result, const char* value, DataType type) {
    char type_str[MAX_TOKEN_LENGTH];
    snprintf(type_str, MAX_TOKEN_LENGTH - 1, "%s", dataTypeToString(type));
    emitTACOp(generator, TAC_OP_CONST, result, value, type_str, NULL);
}

void emitAssignOp(TACGenerator* generator, const char* result, const char* op1) {
    emitTACOp(generator, TAC_OP_ASSIGN, result, op1, NULL, NULL);
}

void emitLabel(TACGenerator* generator, const char* label) {
    emitTACOp(generator, TAC_OP_LABEL, label, NULL, NULL, NULL);
}

void emitGoto(TACGenerator* generator, const char* label) {
    emitTACOp(generator, TAC_OP_GOTO, label, NULL, NULL, NULL);
}

void emitIfGoto(TACGenerator* generator, const char* condition, const char* label) {
    emitTACOp(generator, TAC_OP_IF, label, condition, NULL, NULL);
}

void emitFunction(TACGenerator* generator, const char* func_name) {
    emitTACOp(generator, TAC_OP_FUNC, func_name, NULL, NULL, NULL);
}

void emitReturn(TACGenerator* generator, const char* return_value) {
    emitTACOp(generator, TAC_OP_RETURN, return_value, NULL, NULL, NULL);
}

void emitCall(TACGenerator* generator, const char* result, const char* func_name) {
    emitTACOp(generator, TAC_OP_CALL, result, func_name, NULL, NULL);
}

void emitParam(TACGenerator* generator, const char* param_name) {
    emitTACOp(generator, TAC_OP_PARAM, param_name, NULL, NULL, NULL);
}

char* generateTempVar(TACGenerator* generator) {
    static char temp_var[MAX_TOKEN_LENGTH];
    if (generator == NULL) {
        return NULL;
    }
    snprintf(temp_var, MAX_TOKEN_LENGTH - 1, "_t%d", generator->temp_var_counter++);
    return temp_var;
}

char* generateLabel(TACGenerator* generator) {
    static char label[MAX_TOKEN_LENGTH];
    if (generator == NULL) {
        return NULL;
    }
    snprintf(label, MAX_TOKEN_LENGTH - 1, "L%d", generator->label_counter++);
    return label;
}

void generateTACFromIrNode(TACGenerator* generator, TreeNode* node, SymbolTable* symbol_table) {
    if (generator == NULL || node == NULL) {
        return;
    }

    char label[MAX_TOKEN_LENGTH];
    snprintf(label, MAX_TOKEN_LENGTH - 1, "IR_%d", node->line_number);
    emitLabel(generator, label);

    for (int i = 0; i < node->num_children; i++) {
        generateTACFromNode(generator, node->child[i], symbol_table);
    }
}

void generateTACFromAttributeNode(TACGenerator* generator, TreeNode* node, SymbolTable* symbol_table) {
    if (generator == NULL || node == NULL) {
        return;
    }

    char attr_name[MAX_TOKEN_LENGTH] = {0};
    char attr_value[MAX_TOKEN_LENGTH] = {0};

    for (int i = 0; i < node->num_children; i++) {
        TreeNode* child = node->child[i];

        if (child->node_kind == NODE_IDENTIFIER) {
            strncpy(attr_name, child->value, MAX_TOKEN_LENGTH - 1);
        } else if (child->node_kind == NODE_VALUE_DEF) {
            strncpy(attr_value, child->value, MAX_TOKEN_LENGTH - 1);
        }
    }

    if (attr_name[0] != '\0') {
        emitAttributeOp(generator, attr_name, attr_value);
    }

    for (int i = 0; i < node->num_children; i++) {
        generateTACFromNode(generator, node->child[i], symbol_table);
    }
}

void generateTACFromInputNode(TACGenerator* generator, TreeNode* node, SymbolTable* symbol_table) {
    if (generator == NULL || node == NULL) {
        return;
    }

    char input_name[MAX_TOKEN_LENGTH] = {0};

    for (int i = 0; i < node->num_children; i++) {
        TreeNode* child = node->child[i];

        if (child->node_kind == NODE_IDENTIFIER) {
            strncpy(input_name, child->value, MAX_TOKEN_LENGTH - 1);
        }
    }

    if (input_name[0] != '\0') {
        emitInputOp(generator, input_name);
    }

    for (int i = 0; i < node->num_children; i++) {
        generateTACFromNode(generator, node->child[i], symbol_table);
    }
}

void generateTACFromOutputNode(TACGenerator* generator, TreeNode* node, SymbolTable* symbol_table) {
    if (generator == NULL || node == NULL) {
        return;
    }

    char output_name[MAX_TOKEN_LENGTH] = {0};

    for (int i = 0; i < node->num_children; i++) {
        TreeNode* child = node->child[i];

        if (child->node_kind == NODE_IDENTIFIER) {
            strncpy(output_name, child->value, MAX_TOKEN_LENGTH - 1);
        }
    }

    if (output_name[0] != '\0') {
        emitOutputOp(generator, output_name);
    }

    for (int i = 0; i < node->num_children; i++) {
        generateTACFromNode(generator, node->child[i], symbol_table);
    }
}

void generateTACFromInitializerNode(TACGenerator* generator, TreeNode* node, SymbolTable* symbol_table) {
    if (generator == NULL || node == NULL) {
        return;
    }

    char initializer_name[MAX_TOKEN_LENGTH] = {0};

    for (int i = 0; i < node->num_children; i++) {
        TreeNode* child = node->child[i];

        if (child->node_kind == NODE_IDENTIFIER) {
            strncpy(initializer_name, child->value, MAX_TOKEN_LENGTH - 1);
        }
    }

    if (initializer_name[0] != '\0') {
        emitInitializerOp(generator, initializer_name);
    }

    for (int i = 0; i < node->num_children; i++) {
        generateTACFromNode(generator, node->child[i], symbol_table);
    }
}

void generateTACFromTensorNode(TACGenerator* generator, TreeNode* node, SymbolTable* symbol_table) {
    if (generator == NULL || node == NULL) {
        return;
    }

    char tensor_name[MAX_TOKEN_LENGTH] = {0};
    char data_type[MAX_TOKEN_LENGTH] = {0};

    for (int i = 0; i < node->num_children; i++) {
        TreeNode* child = node->child[i];

        if (child->node_kind == NODE_IDENTIFIER) {
            strncpy(tensor_name, child->value, MAX_TOKEN_LENGTH - 1);
        } else if (child->node_kind == NODE_DATA_TYPE_DEF) {
            strncpy(data_type, child->value, MAX_TOKEN_LENGTH - 1);
        }
    }

    if (tensor_name[0] != '\0') {
        emitTensorOp(generator, tensor_name, data_type);
    }

    for (int i = 0; i < node->num_children; i++) {
        generateTACFromNode(generator, node->child[i], symbol_table);
    }
}

void generateTACFromOpsetImportNode(TACGenerator* generator, TreeNode* node, SymbolTable* symbol_table) {
    if (generator == NULL || node == NULL) {
        return;
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
        char version_str[MAX_TOKEN_LENGTH];
        snprintf(version_str, MAX_TOKEN_LENGTH - 1, "%d", version);
        emitTACOp(generator, TAC_OP_OPSET_IMPORT, domain, version_str, NULL, NULL);
    }

    for (int i = 0; i < node->num_children; i++) {
        generateTACFromNode(generator, node->child[i], symbol_table);
    }
}

void generateTACFromNode(TACGenerator* generator, TreeNode* node, SymbolTable* symbol_table) {
    if (generator == NULL || node == NULL) {
        return;
    }

    switch (node->node_kind) {
        case NODE_ROOT:
        case NODE_IR:
            generateTACFromIrNode(generator, node, symbol_table);
            break;

        case NODE_ATTRIBUTE_REPEATS:
            for (int i = 0; i < node->num_children; i++) {
                generateTACFromAttributeNode(generator, node->child[i], symbol_table);
            }
            break;

        case NODE_ATTRIBUTE_DEF:
            generateTACFromAttributeNode(generator, node, symbol_table);
            break;

        case NODE_INPUT_DEF:
            generateTACFromInputNode(generator, node, symbol_table);
            break;

        case NODE_OUTPUT_DEF:
            generateTACFromOutputNode(generator, node, symbol_table);
            break;

        case NODE_INITIALIZER_DEF:
            generateTACFromInitializerNode(generator, node, symbol_table);
            break;

        case NODE_TENSOR_DEF:
            generateTACFromTensorNode(generator, node, symbol_table);
            break;

        case NODE_OPSET_IMPORT_DEF:
            generateTACFromOpsetImportNode(generator, node, symbol_table);
            break;

        default:
            for (int i = 0; i < node->num_children; i++) {
                generateTACFromNode(generator, node->child[i], symbol_table);
            }
            break;
    }
}

void generateTACFromAST(TACGenerator* generator, TreeNode* root, SymbolTable* symbol_table) {
    if (generator == NULL || root == NULL) {
        return;
    }

    if (listing != NULL) {
        fprintf(listing, "\n============= TAC GENERATION =============\n");
    }

    TreeNode* current = root;
    while (current != NULL) {
        generateTACFromNode(generator, current, symbol_table);
        current = current->sibling;
    }

    if (listing != NULL) {
        fprintf(listing, "============================================\n\n");
    }
}

void printTACInstruction(TACInstruction* instr, FILE* output) {
    if (instr == NULL || output == NULL) {
        return;
    }

    fprintf(output, "%-10s", tacOpToString(instr->op));

    if (instr->result[0] != '\0') {
        fprintf(output, " %s", instr->result);
    }

    if (instr->operand1[0] != '\0') {
        fprintf(output, ", %s", instr->operand1);
    }

    if (instr->operand2[0] != '\0') {
        fprintf(output, ", %s", instr->operand2);
    }

    if (instr->operand3[0] != '\0') {
        fprintf(output, ", %s", instr->operand3);
    }

    if (instr->comment[0] != '\0') {
        fprintf(output, "  ; %s", instr->comment);
    }

    fprintf(output, "\n");
}

void printTAC(TACGenerator* generator, FILE* output) {
    if (generator == NULL || output == NULL) {
        return;
    }

    fprintf(output, "\n============= THREE-ADDRESS CODE =============\n");
    fprintf(output, "Total Instructions: %d\n", generator->instruction_count);
    fprintf(output, "--------------------------------------------\n");

    TACInstruction* current = generator->first;
    while (current != NULL) {
        fprintf(output, "%3d: ", current->label);
        printTACInstruction(current, output);
        current = current->next;
    }

    fprintf(output, "==============================================\n\n");
}

int getTACInstructionCount(TACGenerator* generator) {
    if (generator == NULL) {
        return 0;
    }
    return generator->instruction_count;
}

TACInstruction* getTACFirst(TACGenerator* generator) {
    if (generator == NULL) {
        return NULL;
    }
    return generator->first;
}

TACInstruction* getTACLast(TACGenerator* generator) {
    if (generator == NULL) {
        return NULL;
    }
    return generator->last;
}
