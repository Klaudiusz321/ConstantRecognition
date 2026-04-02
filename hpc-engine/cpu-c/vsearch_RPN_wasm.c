/* vsearch_RPN_wasm.c - WASM wrapper for JS frontend
 *
 * Author: Andrzej Odrzywolek, andrzej.odrzywolek@uj.edu.pl
 * Date: January 2, 2025
 *
 * This file provides:
 *   - String parsing for runtime-configurable calculators
 *   - WASM exported functions for JavaScript frontend
 *   - Backward compatibility with existing web interface
 *
 * Compilation:
 *   emcc -O2 -Wall vsearch_RPN_wasm.c vsearch_RPN_core.c utils.c -s WASM=1 -s EXPORTED_FUNCTIONS='["_search_RPN","_search_RPN_hybrid","_vsearch_RPN","_free"]'  -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]'  -o vsearch.js
 *
 * WebAssembly (emcc, Windows):
 * Install emsdk
 *
 * git clone https://github.com/emscripten-core/emsdk.git
 * cd emsdk
 * emsdk> .\emsdk install latest
 *        .\emsdk activate latest
 */

#ifdef _WIN32
#define strdup _strdup
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vsearch_RPN_core.h"
#include "CALC4.h"

/* ============================================================================
 * STRING-BASED WRAPPER
 * Parses comma-separated strings → calls core with arrays
 * Uses .name fields from CALC4 tables - no duplication!
 * ============================================================================ */

#define MAX_OPS 64

char* vsearch_RPN(
    double z, double dz,
    int MinK, int MaxK,
    int cpu_id, int ncpus,
    const char* const_list,   /* comma-separated, e.g. "PI,EULER,ONE" or NULL for all */
    const char* fun_list,     /* comma-separated, e.g. "LOG,EXP,SQRT" or NULL for all */
    const char* op_list)      /* comma-separated, e.g. "PLUS,TIMES" or NULL for all */
{
    ConstOp const_ops[MAX_OPS];
    UnaryOp unary_ops[MAX_OPS];
    BinaryOp binary_ops[MAX_OPS];
    
    int n_const = 0;
    int n_unary = 0;
    int n_binary = 0;
    
    /* Parse constants (or use all if NULL) */
    if (const_list == NULL) {
        n_const = CALC4_N_CONST;
        for (int i = 0; i < CALC4_N_CONST; i++) {
            const_ops[i] = CALC4_CONSTS[i];
        }
    } else if (const_list[0] != '\0') {
        char* copy = strdup(const_list);
        char* token = strtok(copy, ",");
        while (token != NULL && n_const < MAX_OPS) {
            /* Search by .name field in CALC4_CONSTS */
            for (int i = 0; i < CALC4_N_CONST; i++) {
                if (strcmp(token, CALC4_CONSTS[i].name) == 0) {
                    const_ops[n_const++] = CALC4_CONSTS[i];
                    break;
                }
            }
            token = strtok(NULL, ",");
        }
        free(copy);
    }
    
    /* Parse unary functions (or use all if NULL) */
    if (fun_list == NULL) {
        n_unary = CALC4_N_UNARY;
        for (int i = 0; i < CALC4_N_UNARY; i++) {
            unary_ops[i] = CALC4_FUNCS[i];
        }
    } else if (fun_list[0] != '\0') {
        char* copy = strdup(fun_list);
        char* token = strtok(copy, ",");
        while (token != NULL && n_unary < MAX_OPS) {
            /* Search by .name field in CALC4_FUNCS */
            for (int i = 0; i < CALC4_N_UNARY; i++) {
                if (strcmp(token, CALC4_FUNCS[i].name) == 0) {
                    unary_ops[n_unary++] = CALC4_FUNCS[i];
                    break;
                }
            }
            token = strtok(NULL, ",");
        }
        free(copy);
    }
    
    /* Parse binary operators (or use all if NULL) */
    if (op_list == NULL) {
        n_binary = CALC4_N_BINARY;
        for (int i = 0; i < CALC4_N_BINARY; i++) {
            binary_ops[i] = CALC4_OPS[i];
        }
    } else if (op_list[0] != '\0') {
        char* copy = strdup(op_list);
        char* token = strtok(copy, ",");
        while (token != NULL && n_binary < MAX_OPS) {
            /* Search by .name field in CALC4_OPS */
            for (int i = 0; i < CALC4_N_BINARY; i++) {
                if (strcmp(token, CALC4_OPS[i].name) == 0) {
                    binary_ops[n_binary++] = CALC4_OPS[i];
                    break;
                }
            }
            token = strtok(NULL, ",");
        }
        free(copy);
    }
    
    return search_constant(z, dz, MinK, MaxK, cpu_id, ncpus,
                          const_ops, n_const,
                          unary_ops, n_unary,
                          binary_ops, n_binary,
                          ERROR_REL, COMPARE_STRICT);
}

/* ============================================================================
 * WASM EXPORTED FUNCTIONS
 * ============================================================================ */

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

/* Legacy API: uses full CALC4 calculator */
EMSCRIPTEN_KEEPALIVE
char* search_RPN(double z, double dz, int MinK, int MaxK, int cpu_id, int ncpus) {
    return search_constant(z, dz, MinK, MaxK, cpu_id, ncpus,
                          CALC4_CONSTS, CALC4_N_CONST,
                          CALC4_FUNCS,  CALC4_N_UNARY,
                          CALC4_OPS,    CALC4_N_BINARY,
                          ERROR_REL, COMPARE_STRICT);
}

/* Hybrid search (same as search_RPN for now - placeholder for FP32+FP64) */
EMSCRIPTEN_KEEPALIVE
char* search_RPN_hybrid(double z, double dz, int MinK, int MaxK, int cpu_id, int ncpus) {
    return search_constant(z, dz, MinK, MaxK, cpu_id, ncpus,
                          CALC4_CONSTS, CALC4_N_CONST,
                          CALC4_FUNCS,  CALC4_N_UNARY,
                          CALC4_OPS,    CALC4_N_BINARY,
                          ERROR_REL, COMPARE_STRICT);
}

/* Configurable search via strings */
EMSCRIPTEN_KEEPALIVE
char* search_RPN_custom(double z, double dz, int MinK, int MaxK, int cpu_id, int ncpus,
                        const char* consts, const char* funcs, const char* ops) {
    return vsearch_RPN(z, dz, MinK, MaxK, cpu_id, ncpus, consts, funcs, ops);
}

/* Function recognition via WASM */
EMSCRIPTEN_KEEPALIVE
char* search_function_wasm(
    const double* x_values, const double* y_values, const double* dy_values,
    int n_data,
    int MinK, int MaxK,
    int cpu_id, int ncpus)
{
    /* Convert arrays to DataPoint array */
    DataPoint* data = (DataPoint*)malloc(n_data * sizeof(DataPoint));
    if (!data) {
        return strdup("{\"error\":\"Memory allocation failed\"}");
    }
    
    for (int i = 0; i < n_data; i++) {
        data[i].x = x_values[i];
        data[i].y = y_values[i];
        data[i].dy = (dy_values != NULL) ? dy_values[i] : 0.0;
    }
    
    char* result = search_function(
        data, n_data,
        MinK, MaxK,
        cpu_id, ncpus,
        CALC4_CONSTS, CALC4_N_CONST,
        CALC4_FUNCS,  CALC4_N_UNARY,
        CALC4_OPS,    CALC4_N_BINARY,
        ERROR_MSE, COMPARE_STRICT);
    
    free(data);
    return result;
}

#endif /* __EMSCRIPTEN__ */
