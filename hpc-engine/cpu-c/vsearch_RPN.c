/* vsearch_RPN.c

Configurable RPN expression search for constant recognition
Author: Andrzej Odrzywolek, andrzej.odrzywolek@uj.edu.pl
Code assist: Claude 4.5 Opus, 2025-12-20 

Compilation examples:

  Standalone test (gcc/clang/icx):
    gcc -O2 -Wall -DSTANDALONE_TEST vsearch_RPN.c -lm -o vsearch
    clang -O2 -Wall -DSTANDALONE_TEST vsearch_RPN.c -lm -o vsearch
    icx -O2 -Wall -DSTANDALONE_TEST vsearch_RPN.c -lm -o vsearch

  WebAssembly (emcc, Linux):
    emcc -O2 -Wall vsearch_RPN.c -s WASM=1 -s EXPORTED_FUNCTIONS='["_vsearch_RPN", "_search_RPN", "_search_RPN_hybrid", "_free"]' -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' -o vsearch.js

  WebAssembly (emcc, Windows):
  Install emsdk

  git clone https://github.com/emscripten-core/emsdk.git
  cd emsdk
  emsdk> .\emsdk install latest
         .\emsdk activate latest

  emcc -O2 -Wall vsearch_RPN.c -s WASM=1 -s EXPORTED_FUNCTIONS='["_vsearch_RPN", "_search_RPN", "_search_RPN_hybrid", "_free"]' -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' -o vsearch.js

  Windows (cl.exe from x64 Native Tools Command Prompt for VS 2022):
    cl /O2 /W3 /DSTANDALONE_TEST vsearch_RPN.c /Fe:vsearch_cl.exe

  Windows/Intel (icx.exe)
  Initialize environment:
    cmd.exe "/K" '"C:\Program Files (x86)\Intel\oneAPI\setvars.bat" && pwsh'
    icx -O2 -Wall -DSTANDALONE_TEST vsearch_RPN.c -o vsearch	
 */

/* Windows related settings for icx.exe */
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif


#ifdef _WIN32
#define strdup _strdup
#endif

/* Windows related include for cl.exe */
#ifdef _MSC_VER
#include <intrin.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <float.h>

#include "opcodes.h"

/* ============================================================================
 * CONFIGURATION
 * ============================================================================ */

#define MAX_CODE_LENGTH  32
#define MAX_STACK_DEPTH  32
#define JSON_BUFFER_SIZE (1024 * 1024)
#define EPS_MAX          16    /* Max error in EPSILON units for "exact" match */

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static const char BUILD_TIMESTAMP[] = __DATE__ " " __TIME__;

static const char COMPILER_VERSION[] =
#ifdef __VERSION__
__VERSION__;
#elif defined(__clang_version__)
__clang_version__;
#else
"Unknown";
#endif

static const char ARCH_INFO[] = 
#ifdef __x86_64__
    "x86_64";
#elif defined(__arm__)
    "ARM";
#elif defined(__wasm__)
    "WASM";  
#else
    "Unknown";
#endif
;

static const char OS_INFO[] = 
#ifdef __linux__
    "Linux";
#elif defined(__APPLE__)
    "macOS";
#elif defined(_WIN32)
    "Windows";
#elif defined(__unix__)
    "Unix-like";
#else
    "Unknown";
#endif
;


/* ============================================================================
 * INSTRUCTION SET PARSING
 * 
 * Parse comma-separated name lists into InstructionSet (from opcodes.h)
 * ============================================================================ */


/* Parse comma-separated list of constant names */
static int parse_const_list(const char* list, InstructionSet* iset) {
    if (list == NULL) {
        /* NULL = use all defaults */
        for (int i = 0; i < CONST_COUNT; i++) {
            iset->const_ops[i] = (unsigned char)i;
        }
        return CONST_COUNT;
    }
    if (list[0] == '\0') {
        /* Empty string = empty set */
        return 0;
    }
    
    int count = 0;
    char* copy = strdup(list);
    char* token = strtok(copy, ",");
    
    while (token != NULL && count < MAX_OPS) {

        for (int i = 0; i < CONST_COUNT; i++) {
            if (strcmp(token, CONST_NAMES[i]) == 0) {
                iset->const_ops[count++] = (unsigned char)i;
                break;
            }
        }
        token = strtok(NULL, ",");
    }
    free(copy);
    return count;
}

/* Parse comma-separated list of unary function names */
static int parse_unary_list(const char* list, InstructionSet* iset) {
    if (list == NULL) {
        /* NULL = use all defaults */
        for (int i = 0; i < UNARY_COUNT; i++) {
            iset->unary_ops[i] = (unsigned char)i;
        }
        return UNARY_COUNT;
    }
    if (list[0] == '\0') {
        /* Empty string = empty set */
        return 0;
    }
    
    int count = 0;
    char* copy = strdup(list);
    char* token = strtok(copy, ",");
    
    while (token != NULL && count < MAX_OPS) {

        for (int i = 0; i < UNARY_COUNT; i++) {
            if (strcmp(token, UNARY_NAMES[i]) == 0) {
                iset->unary_ops[count++] = (unsigned char)i;
                break;
            }
        }
        token = strtok(NULL, ",");
    }
    free(copy);
    return count;
}

/* Parse comma-separated list of binary operator names */
static int parse_binary_list(const char* list, InstructionSet* iset) {
    if (list == NULL) {
        /* NULL = use all defaults */
        for (int i = 0; i < BINARY_COUNT; i++) {
            iset->binary_ops[i] = (unsigned char)i;
        }
        return BINARY_COUNT;
    }
    if (list[0] == '\0') {
        /* Empty string = empty set */
        return 0;
    }
    
    int count = 0;
    char* copy = strdup(list);
    char* token = strtok(copy, ",");
    
    while (token != NULL && count < MAX_OPS) {

        for (int i = 0; i < BINARY_COUNT; i++) {
            if (strcmp(token, BINARY_NAMES[i]) == 0) {
                iset->binary_ops[count++] = (unsigned char)i;
                break;
            }
        }
        token = strtok(NULL, ",");
    }
    free(copy);
    return count;
}

/* Build instruction set from string lists (NULL = use all CALC4 defaults) */
static InstructionSet* build_instruction_set(const char* const_list, 
                                              const char* fun_list, 
                                              const char* op_list) {
    InstructionSet* iset = (InstructionSet*)malloc(sizeof(InstructionSet));
    if (!iset) return NULL;
    
    iset->n_const  = parse_const_list(const_list, iset);
    iset->n_unary  = parse_unary_list(fun_list, iset);
    iset->n_binary = parse_binary_list(op_list, iset);
    
    return iset;
}

/* Compute total instruction count */
static inline int iset_total(const InstructionSet* iset) {
    return iset->n_const + iset->n_unary + iset->n_binary;
}

/* ============================================================================
 * TERNARY FORM UTILITIES
 * 
 * Ternary encoding: 0=constant, 1=unary, 2=binary
 * Valid RPN iff stack==1 at end
*
 * Valid RPN iff stack depth remains >=1 throughout and equals 1 at end.
 * The count of valid ternary structures follows Motzkin numbers (OEIS A001006).
 *
 * ============================================================================ */

/* Check if ternary code is syntactically valid RPN */
static int check_ternary_syntax(const char* ternary, int length) {
    int stack = 0;
    for (int i = 0; i < length; i++) {
        switch (ternary[i]) {
            case 0:  /* constant: push */
                stack++;
                break;
            case 1:  /* unary: pop 1, push 1 -> no change */
                if (stack < 1) return 0;
                break;
            case 2:  /* binary: pop 2, push 1 -> stack-- */
                if (stack < 2) return 0;
                stack--;
                break;
        }
    }
    return (stack == 1);
}

/* Convert integer k to ternary representation of length K */
static void int_to_ternary(uint64_t k, char* out, int K) {
    for (int i = K - 1; i >= 0; i--) {
        out[i] = (char)(k % 3);
        k /= 3;
    }
}

/* Increment ternary representation (returns 0 on overflow) */
static int ternary_increment(char* ternary, int K) {
    for (int i = K - 1; i >= 0; i--) {
        ternary[i]++;
        if (ternary[i] < 3) return 1;
        ternary[i] = 0;
    }
    return 0;  /* overflow */
}

/* ============================================================================
 * EXPRESSION EVALUATION
 * ============================================================================ */

/* 
 * Evaluate an expression given:
 *   - ternary[K]: the ternary structure (0/1/2)
 *   - indices[K]: which specific op within each category
 *   - iset: the instruction set mapping
 * 
 * Returns NAN on evaluation error
 */
static double evaluate_expression(const char* ternary, const int* indices, 
                                   int K, const InstructionSet* iset) {
    double stack[MAX_STACK_DEPTH];
    int sp = 0;  /* stack pointer */
    
    for (int i = 0; i < K; i++) {
        switch (ternary[i]) {
            case 0:  /* constant */
                if (sp >= MAX_STACK_DEPTH) return nan("");
                stack[sp++] = CONST_VALUES[iset->const_ops[indices[i]]];
                break;
                
            case 1:  /* unary function */
                if (sp < 1) return nan("");
                stack[sp-1] = apply_unary(iset->unary_ops[indices[i]], stack[sp-1]);
                break;
                
            case 2:  /* binary operator */
                if (sp < 2) return nan("");
                sp--;
                double b = stack[sp];     /* top of stack (second pushed) */
                double a = stack[sp-1];   /* second from top (first pushed) */
                /* Convention: op(top, second) to match JS frontend */
                stack[sp-1] = apply_binary(iset->binary_ops[indices[i]], b, a);
                break;
        }
    }
    
    return (sp == 1) ? stack[0] : nan("");
}

/* ============================================================================
 * CODE FORMATTING
 * ============================================================================ */

/* Format expression as comma-separated Mathematica-style names */
static void format_code(const char* ternary, const int* indices, int K,
                        const InstructionSet* iset, char* out, int out_size) {
    int pos = 0;
    
    for (int i = 0; i < K && pos < out_size - 20; i++) {
        if (i > 0) {
            out[pos++] = ',';
            out[pos++] = ' ';
        }
        
        const char* name = NULL;
        switch (ternary[i]) {
            case 0: name = CONST_NAMES[iset->const_ops[indices[i]]]; break;
            case 1: name = UNARY_NAMES[iset->unary_ops[indices[i]]]; break;
            case 2: name = BINARY_NAMES[iset->binary_ops[indices[i]]]; break;
        }
        
        if (name) {
            int len = strlen(name);
            if (pos + len < out_size) {
                memcpy(out + pos, name, len);
                pos += len;
            }
        }
    }
    out[pos] = '\0';
}

/* ============================================================================
 * HAMMING DISTANCE (cross-platform)
 * ============================================================================ */

static inline int popcount64(uint64_t x) {
#if defined(_MSC_VER)
    /* MSVC */
    return (int)__popcnt64(x);
#elif defined(__GNUC__) || defined(__clang__)
    /* GCC, Clang, ICX */
    return __builtin_popcountll(x);
#else
    /* Portable fallback */
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
    return (int)((x * 0x0101010101010101ULL) >> 56);
#endif
}

static double hamming_distance(double a, double b) {
    uint64_t ua, ub;
    memcpy(&ua, &a, sizeof(double));
    memcpy(&ub, &b, sizeof(double));
    return (double) popcount64(ua ^ ub);
}

/* ============================================================================
 * RECURSIVE COMBINATION GENERATOR
 * 
 * For a given ternary structure, enumerate all concrete expressions
 * ============================================================================ */

typedef struct {
    double target;
    double delta;
    double best_err;
    double best_value;
    int best_K;
    int best_indices[MAX_CODE_LENGTH];
    char best_ternary[MAX_CODE_LENGTH];
    int found_exact;
    
    /* JSON output */
    char* json_ptr;
    int json_remaining;
    int result_count;
    int cpu_id;
    
    /* Instruction set */
    const InstructionSet* iset;
} SearchState;

static int generate_and_evaluate(const char* ternary, int* indices, int pos, int K,
                                  SearchState* state) {
    if (state->found_exact) return 1;
    
    if (pos == K) {
        /* Evaluate complete expression */
        double val = evaluate_expression(ternary, indices, K, state->iset);
        if (isnan(val) || isinf(val)) return 0;
        
        double err = (state->target == 0.0) ? 
                     fabs(val) : fabs(val / state->target - 1.0);
        
        if (err < state->best_err) {
            state->best_err = err;
            state->best_value = val;
            state->best_K = K;
            memcpy(state->best_ternary, ternary, K);
            memcpy(state->best_indices, indices, K * sizeof(int));
            
            /* Format and output intermediate result */
            char code[512];
            format_code(ternary, indices, K, state->iset, code, sizeof(code));
            
            if (state->result_count > 0) {
                int w = snprintf(state->json_ptr, state->json_remaining, ",\n");
                state->json_ptr += w;
                state->json_remaining -= w;
            }
            
            int w = snprintf(state->json_ptr, state->json_remaining,
                "{\"RPN\":\"%s\", \"REL_ERR\":%.17e, \"K\":%d, "
                "\"result\":\"INTERMEDIATE\", \"status\":\"RUNNING\", "
                "\"cpuId\":%d, \"HAMMING_DISTANCE\":%lf}",
                code, err, K, state->cpu_id, hamming_distance(val, state->target));
            state->json_ptr += w;
            state->json_remaining -= w;
            state->result_count++;
            
            /* Check for exact match */
            double compression = (err > 0) ? -log10(err) / (K * log10(iset_total(state->iset))) : 10.0;
            
            if (err <= EPS_MAX * DBL_EPSILON ||
                (fabs(val - state->target) <= 2.0 * state->delta && compression >= 1.05)) {
                state->found_exact = 1;
                return 1;
            }
        }
        return 0;
    }
    
    /* Recurse: try all options for current position */
    int n_options;
    switch (ternary[pos]) {
        case 0: n_options = state->iset->n_const; break;
        case 1: n_options = state->iset->n_unary; break;
        case 2: n_options = state->iset->n_binary; break;
        default: return 0;
    }
    
    for (int i = 0; i < n_options; i++) {
        indices[pos] = i;
        if (generate_and_evaluate(ternary, indices, pos + 1, K, state)) {
            return 1;
        }
    }
    return 0;
}

/* ============================================================================
 * MAIN SEARCH FUNCTION
 * ============================================================================ */

/*
 * vsearch_RPN - Variable instruction set RPN search
 * 
 * Parameters:
 *   z          - target value to recognize
 *   dz         - absolute error tolerance (0 = ignore)
 *   MinK       - minimum code length
 *   MaxK       - maximum code length
 *   cpu_id     - worker ID (0..ncpus-1)
 *   ncpus      - total number of workers
 *   const_list - comma-separated constant names, or NULL for all
 *   fun_list   - comma-separated unary function names, or NULL for all
 *   op_list    - comma-separated binary operator names, or NULL for all
 * 
 * Returns:
 *   JSON string with search results (caller must free)
 */
char* vsearch_RPN(double z, double dz, int MinK, int MaxK, 
                  int cpu_id, int ncpus,
                  const char* const_list, const char* fun_list, const char* op_list) {
    
    /* Build instruction set */
    InstructionSet* iset = build_instruction_set(const_list, fun_list, op_list);
    if (!iset) return strdup("{\"error\":\"Failed to build instruction set\"}");
    
    /* Allocate output buffer */
    char* json_output = (char*)malloc(JSON_BUFFER_SIZE);
    if (!json_output) {
        free(iset);
        return strdup("{\"error\":\"Memory allocation failed\"}");
    }
    
    /* Initialize search state */
    SearchState state = {
        .target = z,
        .delta = dz,
        .best_err = DBL_MAX,
        .best_value = 0.0,
        .best_K = 1,
        .found_exact = 0,
        .json_ptr = json_output,
        .json_remaining = JSON_BUFFER_SIZE,
        .result_count = 0,
        .cpu_id = cpu_id,
        .iset = iset
    };
    
    /* Initialize best to first constant */
    state.best_ternary[0] = 0;
    state.best_indices[0] = 0;
    
    /* Start JSON object */
    int w = snprintf(state.json_ptr, state.json_remaining, "{\n"); state.json_ptr += w; state.json_remaining -= w;

    w = snprintf(state.json_ptr, state.json_remaining, "\"z\":  %.17lf,\n", z);                       state.json_ptr += w; state.json_remaining -= w;
    w = snprintf(state.json_ptr, state.json_remaining, "\"dz\": %.17lf,\n", dz);                      state.json_ptr += w; state.json_remaining -= w;
    w = snprintf(state.json_ptr, state.json_remaining, "\"minK\":  %d,\n", MinK);                     state.json_ptr += w; state.json_remaining -= w;
    w = snprintf(state.json_ptr, state.json_remaining, "\"maxK\":  %d,\n", MaxK);                     state.json_ptr += w; state.json_remaining -= w;
    w = snprintf(state.json_ptr, state.json_remaining, "\"cpuId\": %d,\n", cpu_id);                   state.json_ptr += w; state.json_remaining -= w;
    w = snprintf(state.json_ptr, state.json_remaining, "\"ncpus\": %d,\n", ncpus);                    state.json_ptr += w; state.json_remaining -= w;
    w = snprintf(state.json_ptr, state.json_remaining, "\"buildTime\": \"%s\",\n", BUILD_TIMESTAMP);  state.json_ptr += w; state.json_remaining -= w;
    w = snprintf(state.json_ptr, state.json_remaining, "\"compiler\":  \"%s\",\n", COMPILER_VERSION); state.json_ptr += w; state.json_remaining -= w;
    w = snprintf(state.json_ptr, state.json_remaining, "\"arch\":      \"%s\",\n", ARCH_INFO);        state.json_ptr += w; state.json_remaining -= w;
    w = snprintf(state.json_ptr, state.json_remaining, "\"os\":        \"%s\",\n", OS_INFO);          state.json_ptr += w; state.json_remaining -= w;
    
    /* Start results array */
    w = snprintf(state.json_ptr, state.json_remaining, "\"results\": [\n"); 
    state.json_ptr += w; state.json_remaining -= w;

    /* Ternary code and indices buffers */
    char ternary[MAX_CODE_LENGTH];
    int indices[MAX_CODE_LENGTH];
    
    uint64_t total_ternary = 0;
    uint64_t valid_ternary = 0;
    
    /* Main search loop */
    for (int K = MinK; K <= MaxK && !state.found_exact; K++) {
        uint64_t n_ternary = 1;
        for (int i = 0; i < K; i++) n_ternary *= 3;  /* 3^K */
        
        /* Divide work among CPUs */
        uint64_t chunk = (n_ternary + ncpus - 1) / ncpus;
        uint64_t start = (uint64_t)cpu_id * chunk;
        uint64_t end = MIN(start + chunk, n_ternary);
        
        /* Initialize ternary to starting position */
        int_to_ternary(start, ternary, K);
        
        for (uint64_t t = start; t < end && !state.found_exact; t++) {
            total_ternary++;
            
            if (check_ternary_syntax(ternary, K)) {
                valid_ternary++;
                generate_and_evaluate(ternary, indices, 0, K, &state);
            }
            
            if (t < end - 1) {
                ternary_increment(ternary, K);
            }
        }
        
        /* Output K_BEST after each K level */
        if (!state.found_exact && state.best_K > 0) {
            char code[512];
            format_code(state.best_ternary, state.best_indices, state.best_K, 
                       iset, code, sizeof(code));
            
            if (state.result_count > 0) {
                w = snprintf(state.json_ptr, state.json_remaining, ",");
                state.json_ptr += w;
                state.json_remaining -= w;
            }
            
            w = snprintf(state.json_ptr, state.json_remaining,
                "{\"RPN\":\"%s\", \"REL_ERR\":%.17e, \"K\":%d, "
                "\"result\":\"K_BEST\", \"status\":\"RUNNING\", "
                "\"cpuId\":%d, \"HAMMING_DISTANCE\":%lf}",
                code, state.best_err, K, cpu_id, 
                hamming_distance(state.best_value, z));
            state.json_ptr += w;
            state.json_remaining -= w;
            state.result_count++;
        }
        
        /* Early abort: nothing found after many iterations */
        if (valid_ternary <= 12 && total_ternary > 250 && K > 4) {
            char code[512];
            format_code(state.best_ternary, state.best_indices, state.best_K,
                       iset, code, sizeof(code));
            
            w = snprintf(state.json_ptr, state.json_remaining,
                "], \"result\":\"ABORTED\", \"RPN\":\"%s\", \"REL_ERR\":%.17e, "
                "\"K\":%d, \"status\":\"FINISHED\", \"HAMMING_DISTANCE\":%lf}",
                code, state.best_err, state.best_K,
                hamming_distance(state.best_value, z));
            
            free(iset);
            return json_output;
        }
    }
    
    /* Finalize JSON */
    char code[512];
    format_code(state.best_ternary, state.best_indices, state.best_K,
               iset, code, sizeof(code));
    
    /* Calculate compression ratio */
    double compression;
    int n_total = iset_total(iset);
    
    if (z == 0.0) {
        compression = 0.0;  /* Handle cases where target is 0 */
    } else {
        if (state.best_err == 0.0) {
            /* Perfect match (special case) */
            double digitsInTarget = floor(log10(fabs(z))) + 1.0;
            double informationInRPN = state.best_K * log10(n_total);
            
            if (informationInRPN <= 0.0) {
                compression = 0.0;
            } else {
                compression = digitsInTarget / informationInRPN;
            }
        } else {
            /* General case (approximation) */
            double digitsInTarget = -log10(state.best_err);
            double informationInRPN = state.best_K * log10(n_total);
            
            if (state.best_err >= 1.0 || informationInRPN <= 0.0) {
                compression = 0.0;
            } else {
                compression = digitsInTarget / informationInRPN;
            }
        }
    }
    
    const char* result_type = state.found_exact ? "SUCCESS" : "FAILURE";
    
    w = snprintf(state.json_ptr, state.json_remaining,
        "], \"result\":\"%s\", \"RPN\":\"%s\", \"REL_ERR\":%.17e, "
        "\"INPUT_ABS_ERR\":%lf, \"COMPRESSION_RATIO\":%lf, \"K\":%d, "
        "\"status\":\"FINISHED\", \"HAMMING_DISTANCE\":%lf}",
        result_type, code, state.best_err, dz, compression, state.best_K,
        hamming_distance(state.best_value, z));
    
    free(iset);
    return json_output;
}

/* ============================================================================
 * MAIN (for standalone testing)
 * ============================================================================ */

#ifdef STANDALONE_TEST

int main(int argc, char** argv) {

    double z = 77777;
    double dz = 0.01;
    int MaxK = 6;
    int cpu_id = 0;
    int ncpus = 1;

    const char* consts = NULL;
    const char* funcs = NULL;
    const char* ops = NULL;
    
    if (argc > 1) sscanf(argv[1], "%lf", &z);
    if (argc > 2) sscanf(argv[2], "%d", &MaxK);
    if (argc > 3) sscanf(argv[3], "%d", &cpu_id);
    if (argc > 4) sscanf(argv[4], "%d", &ncpus);
    if (argc > 5) consts = argv[5];
    if (argc > 6) funcs = argv[6];
    if (argc > 7) ops = argv[7];
    
    char* result = vsearch_RPN(z, dz, 1, MaxK, cpu_id, ncpus, consts, funcs, ops);
    printf("%s\n", result);
    free(result);
    
    return 0;
}

#endif /* STANDALONE_TEST */

/* ============================================================================
 * WASM COMPATIBILITY WRAPPERS
 * 
 * Provide backward-compatible function names for existing frontend
 * ============================================================================ */

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

/* Export with proper WASM visibility */
EMSCRIPTEN_KEEPALIVE
char* search_RPN(double z, double dz, int MinK, int MaxK, int cpu_id, int ncpus) {
    return vsearch_RPN(z, dz, MinK, MaxK, cpu_id, ncpus, NULL, NULL, NULL);
}

/* Hybrid alias for worker.js compatibility */
EMSCRIPTEN_KEEPALIVE
char* search_RPN_hybrid(double z, double dz, int MinK, int MaxK, int cpu_id, int ncpus) {
    return vsearch_RPN(z, dz, MinK, MaxK, cpu_id, ncpus, NULL, NULL, NULL);
}

#endif /* __EMSCRIPTEN__ */
