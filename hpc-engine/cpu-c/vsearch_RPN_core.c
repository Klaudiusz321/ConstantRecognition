/* vsearch_RPN_core.c - Unified constant, function, and batch recognition
 *
 * Author: Andrzej Odrzywolek, andrzej.odrzywolek@uj.edu.pl
 * Date: January 3, 2025
 * Code assist: Claude 4.5 Opus
 *
 * Unified search modes:
 *   - MODE_CONSTANT: n_data=1, no variable x, num_to_find=1
 *   - MODE_FUNCTION: variable x in formulas, one formula fits all data points
 *   - MODE_BATCH: multiple targets, one formula per target, stop after num_to_find
 *
 * MODE_CONSTANT is internally treated as MODE_BATCH with n_data=1.
 *
 * Shared code:
 *   - Ternary form enumeration (Motzkin numbers)
 *   - Recursive generator structure  
 *   - Expression evaluation (with variable support)
 *   - Code formatting
 *   - JSON output
 *   - Stop criteria heuristics
 *
 * Compilation:
 *
 *   Standalone test (gcc/clang/icx):
 *     gcc -O2 -Wall main_vsearch_test.c vsearch_RPN_core.c utils.c -lm -o vsearch_test
 *
 *   WebAssembly (emcc):
 *     emcc -O2 -Wall vsearch_RPN_wasm.c vsearch_RPN_core.c utils.c -s WASM=1 \
 *          -s EXPORTED_FUNCTIONS='["_search_RPN","_search_RPN_hybrid","_vsearch_RPN","_free"]' \
 *          -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' -o vsearch.js
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <float.h>

#include "vsearch_RPN_core.h"
#include "utils.h"

/* ============================================================================
 * CONFIGURATION
 * ============================================================================ */

#define MAX_CODE_LENGTH  32
#define MAX_STACK_DEPTH  32
#define JSON_BUFFER_SIZE (1024 * 1024)
#define EPS_MAX          16    /* Maximum ULPs for "exact" match */

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* ============================================================================
 * BUILD INFO
 * ============================================================================ */

static const char BUILD_TIMESTAMP[] = __DATE__ " " __TIME__;
static const char COMPILER_VERSION[] =
#ifdef __VERSION__
    __VERSION__;
#elif defined(_MSC_VER)
    "MSVC";
#else
    "Unknown";
#endif

static const char ARCH_INFO[] = 
#ifdef __x86_64__
    "x86_64";
#elif defined(__aarch64__)
    "ARM64";
#elif defined(__wasm__)
    "WASM";  
#else
    "Unknown";
#endif

static const char OS_INFO[] = 
#ifdef __linux__
    "Linux";
#elif defined(__APPLE__)
    "macOS";
#elif defined(_WIN32)
    "Windows";
#elif defined(__EMSCRIPTEN__)
    "Emscripten";
#else
    "Unknown";
#endif

/* ============================================================================
 * TERNARY UTILITIES
 *
 * Ternary encoding: 0=constant, 1=unary, 2=binary
 * Valid RPN iff stack depth remains >=1 throughout and equals 1 at end.
 * The count of valid ternary structures follows Motzkin numbers (OEIS A001006).
 * ============================================================================ */

static int check_ternary_syntax(const char* ternary, int length) {
    int stack = 0;
    for (int i = 0; i < length; i++) {
        switch (ternary[i]) {
            case 0: stack++; break;
            case 1: if (stack < 1) return 0; break;
            case 2: if (stack < 2) return 0; stack--; break;
        }
    }
    return (stack == 1);
}

static void int_to_ternary(uint64_t k, char* out, int K) {
    for (int i = K - 1; i >= 0; i--) {
        out[i] = (char)(k % 3);
        k /= 3;
    }
}

static int ternary_increment(char* ternary, int K) {
    for (int i = K - 1; i >= 0; i--) {
        ternary[i]++;
        if (ternary[i] < 3) return 1;
        ternary[i] = 0;
    }
    return 0;
}

/* ============================================================================
 * UNIFIED EXPRESSION EVALUATION
 *
 * MODE_CONSTANT/MODE_BATCH: indices[i] ∈ [0, n_const-1] → constants only
 * MODE_FUNCTION: indices[i] ∈ [0, n_const] → 0=x, 1..n_const=constants
 * ============================================================================ */

static double evaluate_expression(
    const char* ternary, const int* indices, int K,
    const ConstOp* const_ops, int n_const,
    const UnaryOp* unary_ops,
    const BinaryOp* binary_ops,
    SearchMode mode,
    double x_value)   /* Only used in MODE_FUNCTION */
{
    double stack[MAX_STACK_DEPTH];
    int sp = 0;
    
    for (int i = 0; i < K; i++) {
        switch (ternary[i]) {
            case 0:  /* Constant (or variable in function mode) */
                if (sp >= MAX_STACK_DEPTH) return nan("");
                if (mode == MODE_FUNCTION && indices[i] == 0) {
                    stack[sp++] = x_value;  /* Index 0 = variable x */
                } else {
                    /* In function mode, constant indices are shifted by 1 */
                    int idx = (mode == MODE_FUNCTION) ? indices[i] - 1 : indices[i];
                    stack[sp++] = const_ops[idx].value;
                }
                break;
                
            case 1:  /* Unary function */
                if (sp < 1) return nan("");
                stack[sp-1] = unary_ops[indices[i]].func(stack[sp-1]);
                break;
                
            case 2:  /* Binary operator */
                if (sp < 2) return nan("");
                sp--;
                {
                    double b = stack[sp];
                    double a = stack[sp-1];
                    stack[sp-1] = binary_ops[indices[i]].func(b, a);
                }
                break;
        }
    }
    
    return (sp == 1) ? stack[0] : nan("");
}

/* ============================================================================
 * UNIFIED CODE FORMATTING
 * ============================================================================ */

static void format_code(
    const char* ternary, const int* indices, int K,
    const ConstOp* const_ops,
    const UnaryOp* unary_ops,
    const BinaryOp* binary_ops,
    SearchMode mode,
    char* out, int out_size)
{
    int pos = 0;
    
    for (int i = 0; i < K && pos < out_size - 20; i++) {
        if (i > 0) {
            out[pos++] = ',';
            out[pos++] = ' ';
        }
        
        const char* name = NULL;
        switch (ternary[i]) {
            case 0:
                if (mode == MODE_FUNCTION && indices[i] == 0) {
                    name = "x";
                } else {
                    int idx = (mode == MODE_FUNCTION) ? indices[i] - 1 : indices[i];
                    name = const_ops[idx].name;
                }
                break;
            case 1: 
                name = unary_ops[indices[i]].name; 
                break;
            case 2: 
                name = binary_ops[indices[i]].name; 
                break;
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
 * ERROR COMPUTATION
 * ============================================================================ */

static double compute_single_error(double computed, double target, ErrorMetric metric) {
    if (isnan(computed) || isinf(computed)) return DBL_MAX;
    switch (metric) {
        case ERROR_ABS: return fabs(computed - target);
        case ERROR_REL: return (target == 0.0) ? fabs(computed) : fabs(computed/target - 1.0);
        case ERROR_ULP: return (double)compute_ULP_distance(target, computed);
        case ERROR_HAMMING: return (double)compute_hamming_distance(target, computed);
        default: return fabs(computed - target);
    }
}

static int is_exact_match(double err, double computed, double target, double delta, int K, int n_total) {
    if (err <= EPS_MAX * DBL_EPSILON) return 1;
    if (delta > 0) {
        double compression = (err > 0) ? -log10(err) / (K * log10(n_total)) : 10.0;
        if (fabs(computed - target) <= 2.0 * delta && compression >= 1.05) return 1;
    }
    return 0;
}

/* Check if expression contains variable x (MODE_FUNCTION only) */
static int contains_variable(const char* ternary, const int* indices, int K) {
    for (int i = 0; i < K; i++) {
        if (ternary[i] == 0 && indices[i] == 0) return 1;
    }
    return 0;
}

/* Compute MSE/MAE for MODE_FUNCTION (one formula, multiple data points) */
static double compute_function_error(
    const char* ternary, const int* indices, int K,
    const ConstOp* const_ops, int n_const,
    const UnaryOp* unary_ops, const BinaryOp* binary_ops,
    const DataPoint* data, int n_data, ErrorMetric metric)
{
    double error = 0.0, max_err = 0.0;
    int valid = 0;
    for (int i = 0; i < n_data; i++) {
        double computed = evaluate_expression(ternary, indices, K, const_ops, n_const, unary_ops, binary_ops, MODE_FUNCTION, data[i].x);
        if (isnan(computed) || isinf(computed)) { error += 1e10; valid++; continue; }
        double diff = computed - data[i].y;
        double abs_diff = fabs(diff);
        switch (metric) {
            case ERROR_MSE: error += diff * diff; break;
            case ERROR_MAE: case ERROR_ABS: error += abs_diff; break;
            case ERROR_MAX: if (abs_diff > max_err) max_err = abs_diff; break;
            case ERROR_REL: error += (data[i].y == 0.0) ? abs_diff : fabs(computed/data[i].y - 1.0); break;
            default: error += abs_diff;
        }
        valid++;
    }
    if (valid == 0) return DBL_MAX;
    return (metric == ERROR_MAX) ? max_err : error / valid;
}

/* ============================================================================
 * PER-TARGET STATE (for MODE_CONSTANT and MODE_BATCH)
 * ============================================================================ */

typedef struct {
    int found;
    double best_err;
    double computed_value;
    int best_K;
    int best_indices[MAX_CODE_LENGTH];
    char best_ternary[MAX_CODE_LENGTH];
} TargetState;

/* ============================================================================
 * UNIFIED SEARCH STATE
 * ============================================================================ */

typedef struct {
    SearchMode mode;
    const DataPoint* data;
    int n_data;
    ErrorMetric metric;
    CompareMode compare;
    const ConstOp* const_ops;
    int n_const;
    const UnaryOp* unary_ops;
    int n_unary;
    const BinaryOp* binary_ops;
    int n_binary;
    int n_total;
    int num_to_find;
    int num_found;
    int stop_search;
    TargetState* targets;      /* Per-target state for CONSTANT/BATCH */
    double func_best_err;      /* Best error for FUNCTION mode */
    double func_best_value;
    int func_best_K;
    int func_best_indices[MAX_CODE_LENGTH];
    char func_best_ternary[MAX_CODE_LENGTH];
    char* json_ptr;
    int json_remaining;
    int result_count;
    int cpu_id;
    uint64_t total_ternary;
    uint64_t valid_ternary;
    uint64_t evaluations;
} SearchState;

/* ============================================================================
 * UNIFIED RECURSIVE GENERATOR
 * ============================================================================ */

static int generate_and_evaluate(const char* ternary, int* indices, int pos, int K, SearchState* st) {
    if (st->stop_search) return 1;
    
    if (pos == K) {
        st->evaluations++;
        
        if (st->mode == MODE_FUNCTION) {
            /* MODE_FUNCTION: one formula fits all data points */
            if (!contains_variable(ternary, indices, K)) return 0;
            double err = compute_function_error(ternary, indices, K, st->const_ops, st->n_const, st->unary_ops, st->binary_ops, st->data, st->n_data, st->metric);
            int is_better = (st->compare == COMPARE_STRICT) ? (err < st->func_best_err) : (err <= st->func_best_err);
            if (is_better) {
                st->func_best_err = err;
                st->func_best_K = K;
                st->func_best_value = evaluate_expression(ternary, indices, K, st->const_ops, st->n_const, st->unary_ops, st->binary_ops, MODE_FUNCTION, st->data[0].x);
                memcpy(st->func_best_ternary, ternary, K);
                memcpy(st->func_best_indices, indices, K * sizeof(int));
                char code[512];
                format_code(ternary, indices, K, st->const_ops, st->unary_ops, st->binary_ops, MODE_FUNCTION, code, sizeof(code));
                
                if (st->result_count > 0) {
                    int w = snprintf(st->json_ptr, st->json_remaining, ",\n");
                    st->json_ptr += w;
                    st->json_remaining -= w;
                }
                int w = snprintf(st->json_ptr, st->json_remaining,
                    "{"
                    "\"K\":%d, "
                    "\"REL_ERR\":%.5e, "
                    "\"result\":\"INTERMEDIATE\", "
                    "\"status\":\"RUNNING\", "
                    "\"cpuId\":%d, "
                    "\"RPN\":\"%s\""
                    "}",
                    K, err, st->cpu_id, code);
                st->json_ptr += w;
                st->json_remaining -= w;
                st->result_count++;
                
                if (err < 1e-12) {
                    st->stop_search = 1;
                    return 1;
                }
            }
            return 0;
        }
        
        /* MODE_CONSTANT or MODE_BATCH: evaluate once, check against each unfound target */
        double computed = evaluate_expression(ternary, indices, K, st->const_ops, st->n_const, st->unary_ops, st->binary_ops, MODE_CONSTANT, 0.0);
        if (isnan(computed) || isinf(computed)) return 0;
        
        for (int t = 0; t < st->n_data; t++) {
            if (st->targets[t].found) continue;
            double target = st->data[t].y;
            double delta = st->data[t].dy;
            double err = compute_single_error(computed, target, st->metric);
            int is_better = (st->compare == COMPARE_STRICT) ? (err < st->targets[t].best_err) : (err <= st->targets[t].best_err);
            if (is_better) {
                st->targets[t].best_err = err;
                st->targets[t].best_K = K;
                st->targets[t].computed_value = computed;
                memcpy(st->targets[t].best_ternary, ternary, K);
                memcpy(st->targets[t].best_indices, indices, K * sizeof(int));
                
                /* Output INTERMEDIATE result */
                char code[512];
                format_code(ternary, indices, K, st->const_ops, st->unary_ops, st->binary_ops, MODE_CONSTANT, code, sizeof(code));
                int hamming = compute_hamming_distance(target, computed);
                
                if (st->result_count > 0) {
                    int w = snprintf(st->json_ptr, st->json_remaining, ",\n");
                    st->json_ptr += w;
                    st->json_remaining -= w;
                }
                int w = snprintf(st->json_ptr, st->json_remaining,
                    "{"
                    "\"K\":%d, "
                    "\"REL_ERR\":%.5e, "
                    "\"result\":\"INTERMEDIATE\", "
                    "\"status\":\"RUNNING\", "
                    "\"cpuId\":%d, "
                    "\"HAMMING_DISTANCE\":%d, "
                    "\"RPN\":\"%s\""
                    "}",
                    K, err, st->cpu_id, hamming, code);
                st->json_ptr += w;
                st->json_remaining -= w;
                st->result_count++;
            }
            if (is_exact_match(err, computed, target, delta, K, st->n_total)) {
                st->targets[t].found = 1;
                st->num_found++;
                
                /* For batch mode (n_data > 1), output SUCCESS entry in results array.
                   For single-target (CONSTANT mode), skip to preserve backward compatibility. */
                if (st->n_data > 1) {
                    char code[512];
                    format_code(ternary, indices, K, st->const_ops, st->unary_ops, st->binary_ops, MODE_CONSTANT, code, sizeof(code));
                    int hamming = compute_hamming_distance(target, computed);
                    
                    if (st->result_count > 0) {
                        int w = snprintf(st->json_ptr, st->json_remaining, ",\n");
                        st->json_ptr += w;
                        st->json_remaining -= w;
                    }
                    int w = snprintf(st->json_ptr, st->json_remaining,
                        "{"
                        "\"target_id\":%.0f, "
                        "\"target\":%.17g, "
                        "\"K\":%d, "
                        "\"REL_ERR\":%.5e, "
                        "\"result\":\"SUCCESS\", "
                        "\"HAMMING_DISTANCE\":%d, "
                        "\"RPN\":\"%s\""
                        "}",
                        st->data[t].x, target, K, err, hamming, code);
                    st->json_ptr += w;
                    st->json_remaining -= w;
                    st->result_count++;
                }
                
                if (st->num_to_find > 0 && st->num_found >= st->num_to_find) {
                    st->stop_search = 1;
                    return 1;
                }
                break; /* One formula matches ONE target - enables finding multiple formulas for same value */
            }
        }
        return 0;
    }
    
    /* Recursion: determine options for this position */
    int n_options;
    switch (ternary[pos]) {
        case 0: n_options = (st->mode == MODE_FUNCTION) ? st->n_const + 1 : st->n_const; break;
        case 1: n_options = st->n_unary; break;
        case 2: n_options = st->n_binary; break;
        default: return 0;
    }
    for (int i = 0; i < n_options; i++) {
        indices[pos] = i;
        if (generate_and_evaluate(ternary, indices, pos + 1, K, st)) return 1;
    }
    return 0;
}

/* ============================================================================
 * UNIFIED CORE SEARCH FUNCTION
 * ============================================================================ */

char* vsearch_core(
    SearchMode mode,
    const DataPoint* data, int n_data,
    int MinK, int MaxK,
    int cpu_id, int ncpus,
    const ConstOp* const_ops, int n_const,
    const UnaryOp* unary_ops, int n_unary,
    const BinaryOp* binary_ops, int n_binary,
    ErrorMetric metric,
    CompareMode compare,
    int num_to_find)
{
    char* json_output = (char*)malloc(JSON_BUFFER_SIZE);
    if (!json_output) return strdup("{\"error\":\"Memory allocation failed\"}");
    
    /* For MODE_CONSTANT/MODE_BATCH: allocate per-target state */
    TargetState* targets = NULL;
    if (mode != MODE_FUNCTION) {
        targets = (TargetState*)calloc(n_data, sizeof(TargetState));
        if (!targets) { free(json_output); return strdup("{\"error\":\"Memory allocation failed\"}"); }
        for (int i = 0; i < n_data; i++) { targets[i].best_err = DBL_MAX; targets[i].best_K = 1; }
    }
    
    int n_total = n_const + n_unary + n_binary;
    int effective_num = (num_to_find <= 0) ? n_data : num_to_find;
    if (mode == MODE_FUNCTION) effective_num = 1;
    
    SearchState st = {0};
    st.mode = mode;
    st.data = data; st.n_data = n_data;
    st.metric = metric; st.compare = compare;
    st.const_ops = const_ops; st.n_const = n_const;
    st.unary_ops = unary_ops; st.n_unary = n_unary;
    st.binary_ops = binary_ops; st.n_binary = n_binary;
    st.n_total = n_total;
    st.num_to_find = effective_num;
    st.targets = targets;
    st.func_best_err = DBL_MAX; st.func_best_K = 1;
    st.json_ptr = json_output; st.json_remaining = JSON_BUFFER_SIZE;
    st.cpu_id = cpu_id;
    
    /* JSON header */
    const char* mode_str = (mode == MODE_FUNCTION) ? "FUNCTION" : (n_data == 1 ? "CONSTANT" : "BATCH");
    const char* metric_str[] = {"ABS", "REL", "MSE", "MAE", "MAX", "ULP", "HAMMING"};
    const char* compare_str = (compare == COMPARE_STRICT) ? "STRICT" : "EQUAL";
    
    int w = snprintf(st.json_ptr, st.json_remaining,
        "{\n"
        "\"buildTime\": \"%s\",\n"
        "\"mode\": \"%s\",\n"
        "\"metric\": \"%s\",\n"
        "\"compare\": \"%s\",\n"
        "\"n_data\": %d,\n"
        "\"target\": %.17g,\n"
        "\"delta\": %.17g,\n"
        "\"num_to_find\": %d,\n"
        "\"cpuId\": %d,\n"
        "\"ncpus\": %d,\n"
        "\"minK\": %d,\n"
        "\"maxK\": %d,\n"
        "\"n_const\": %d,\n"
        "\"n_unary\": %d,\n"
        "\"n_binary\": %d,\n"
        "\"n_total\": %d,\n"
        "\"compiler\": \"%s\",\n"
        "\"arch\": \"%s\",\n"
        "\"os\": \"%s\",\n"
        "\"results\": [\n",
        BUILD_TIMESTAMP, mode_str, metric_str[metric], compare_str,
        n_data, data[0].y, data[0].dy, effective_num,
        cpu_id, ncpus, MinK, MaxK,
        n_const, n_unary, n_binary, n_total,
        COMPILER_VERSION, ARCH_INFO, OS_INFO);
    st.json_ptr += w;
    st.json_remaining -= w;
    
    char ternary[MAX_CODE_LENGTH];
    int indices[MAX_CODE_LENGTH];
    
    for (int K = MinK; K <= MaxK && !st.stop_search; K++) {
        uint64_t n_ternary = 1;
        for (int i = 0; i < K; i++) n_ternary *= 3;
        uint64_t chunk = (n_ternary + ncpus - 1) / ncpus;
        uint64_t start = (uint64_t)cpu_id * chunk;
        uint64_t end = MIN(start + chunk, n_ternary);
        int_to_ternary(start, ternary, K);
        
        for (uint64_t t = start; t < end && !st.stop_search; t++) {
            st.total_ternary++;
            if (check_ternary_syntax(ternary, K)) {
                st.valid_ternary++;
                generate_and_evaluate(ternary, indices, 0, K, &st);
            }
            if (t < end - 1) ternary_increment(ternary, K);
        }
        
        /* Emit K_BEST after each level (for CONSTANT/BATCH mode) */
        if (!st.stop_search && mode != MODE_FUNCTION) {
            for (int i = 0; i < n_data; i++) {
                if (targets[i].found) continue;
                if (targets[i].best_K > 0) {
                    char code[512];
                    format_code(targets[i].best_ternary, targets[i].best_indices, targets[i].best_K,
                               const_ops, unary_ops, binary_ops, MODE_CONSTANT, code, sizeof(code));
                    int hamming = compute_hamming_distance(data[i].y, targets[i].computed_value);
                    
                    if (st.result_count > 0) {
                        w = snprintf(st.json_ptr, st.json_remaining, ",\n");
                        st.json_ptr += w;
                        st.json_remaining -= w;
                    }
                    w = snprintf(st.json_ptr, st.json_remaining,
                        "{"
                        "\"K\":%d, "
                        "\"REL_ERR\":%.5e, "
                        "\"result\":\"K_BEST\"      , "
                        "\"status\":\"RUNNING\", "
                        "\"cpuId\":%d, "
                        "\"HAMMING_DISTANCE\":%d, "
                        "\"RPN\":\"%s\""
                        "}",
                        K, targets[i].best_err, cpu_id, hamming, code);
                    st.json_ptr += w;
                    st.json_remaining -= w;
                    st.result_count++;
                }
            }
        }
        
        /* Early abort heuristic */
        if (st.valid_ternary <= 12 && st.total_ternary > 250 && K > 4) {
            st.stop_search = 1;
            break;
        }
    }
    
    /* Finalize JSON */
    if (mode == MODE_FUNCTION) {
        char code[512];
        format_code(st.func_best_ternary, st.func_best_indices, st.func_best_K, const_ops, unary_ops, binary_ops, MODE_FUNCTION, code, sizeof(code));
        const char* result_type = (st.func_best_err < 1e-12) ? "SUCCESS" : "FAILURE";
        w = snprintf(st.json_ptr, st.json_remaining,
            "],\n \"result\":\"%s\", \"RPN\":\"%s\", \"REL_ERR\":%.17e, "
            "\"K\":%d, \"status\":\"FINISHED\", "
            "\"total_ternary\":%llu, \"valid_ternary\":%llu, \"evaluations\":%llu}",
            result_type, code, st.func_best_err, st.func_best_K,
            (unsigned long long)st.total_ternary,
            (unsigned long long)st.valid_ternary,
            (unsigned long long)st.evaluations);
    } else {
        /* Output not-found targets with their best approximation.
           For single-target (CONSTANT mode), skip to preserve backward compatibility. */
        int not_found = 0;
        for (int i = 0; i < n_data; i++) {
            if (!targets[i].found) {
                not_found++;
                if (n_data > 1) {  /* Only for batch mode */
                    char code[512];
                    format_code(targets[i].best_ternary, targets[i].best_indices, targets[i].best_K, const_ops, unary_ops, binary_ops, MODE_CONSTANT, code, sizeof(code));
                    int hamming = compute_hamming_distance(data[i].y, targets[i].computed_value);
                    
                    if (st.result_count > 0) {
                        w = snprintf(st.json_ptr, st.json_remaining, ",\n");
                        st.json_ptr += w;
                        st.json_remaining -= w;
                    }
                    w = snprintf(st.json_ptr, st.json_remaining,
                        "{"
                        "\"target_id\":%.0f, "
                        "\"target\":%.17g, "
                        "\"K\":%d, "
                        "\"REL_ERR\":%.5e, "
                        "\"result\":\"BEST\", "
                        "\"HAMMING_DISTANCE\":%d, "
                        "\"RPN\":\"%s\""
                        "}",
                        data[i].x, data[i].y, targets[i].best_K, targets[i].best_err, hamming, code);
                    st.json_ptr += w;
                    st.json_remaining -= w;
                    st.result_count++;
                }
            }
        }
        
        /* For backward compatibility, use first target's best result in final summary */
        char final_code[512];
        format_code(targets[0].best_ternary, targets[0].best_indices, targets[0].best_K,
                   const_ops, unary_ops, binary_ops, MODE_CONSTANT, final_code, sizeof(final_code));
        int final_hamming = compute_hamming_distance(data[0].y, targets[0].computed_value);
        
        /* Compute compression ratio */
        double compression;
        double target_val = data[0].y;
        double delta_val = data[0].dy;
        if (target_val == 0.0) {
            compression = 0.0;
        } else if (targets[0].best_err == 0.0) {
            double digitsInTarget = floor(log10(fabs(target_val))) + 1.0;
            double informationInRPN = targets[0].best_K * log10(n_total);
            compression = (informationInRPN <= 0.0) ? 0.0 : digitsInTarget / informationInRPN;
        } else {
            double digitsInTarget = -log10(targets[0].best_err);
            double informationInRPN = targets[0].best_K * log10(n_total);
            compression = (targets[0].best_err >= 1.0 || informationInRPN <= 0.0) ? 
                         0.0 : digitsInTarget / informationInRPN;
        }
        
        const char* result_type = (st.num_found == n_data) ? "SUCCESS" : ((st.num_found > 0) ? "PARTIAL" : "FAILURE");
        w = snprintf(st.json_ptr, st.json_remaining,
            "],\n \"result\":\"%s\", \"RPN\":\"%s\", \"REL_ERR\":%.17e, "
            "\"INPUT_ABS_ERR\":%lf, \"COMPRESSION_RATIO\":%lf, \"K\":%d, "
            "\"status\":\"FINISHED\", \"HAMMING_DISTANCE\":%d, "
            "\"num_found\":%d, \"num_not_found\":%d, "
            "\"total_ternary\":%llu, \"valid_ternary\":%llu, \"evaluations\":%llu}",
            result_type, final_code, targets[0].best_err,
            delta_val, compression, targets[0].best_K,
            final_hamming, st.num_found, not_found,
            (unsigned long long)st.total_ternary,
            (unsigned long long)st.valid_ternary,
            (unsigned long long)st.evaluations);
        free(targets);
    }
    
    return json_output;
}

/* ============================================================================
 * PUBLIC API WRAPPERS
 * ============================================================================ */

char* search_constant(
    double target, double delta,
    int MinK, int MaxK,
    int cpu_id, int ncpus,
    const ConstOp* const_ops, int n_const,
    const UnaryOp* unary_ops, int n_unary,
    const BinaryOp* binary_ops, int n_binary,
    ErrorMetric metric,
    CompareMode compare)
{
    DataPoint data[1] = {{.x = 0.0, .y = target, .dy = delta}};
    return vsearch_core(MODE_CONSTANT, data, 1, MinK, MaxK, cpu_id, ncpus,
                       const_ops, n_const, unary_ops, n_unary, binary_ops, n_binary,
                       metric, compare, 1);
}

/* ============================================================================
 * PUBLIC API: FUNCTION RECOGNITION
 * ============================================================================ */

char* search_function(
    const DataPoint* data, int n_data,
    int MinK, int MaxK,
    int cpu_id, int ncpus,
    const ConstOp* const_ops, int n_const,
    const UnaryOp* unary_ops, int n_unary,
    const BinaryOp* binary_ops, int n_binary,
    ErrorMetric metric,
    CompareMode compare)
{
    return vsearch_core(MODE_FUNCTION, data, n_data, MinK, MaxK, cpu_id, ncpus,
                       const_ops, n_const, unary_ops, n_unary, binary_ops, n_binary,
                       metric, compare, 1);
}

char* search_batch(
    const DataPoint* data, int n_data,
    int num_to_find,
    int MinK, int MaxK,
    int cpu_id, int ncpus,
    const ConstOp* const_ops, int n_const,
    const UnaryOp* unary_ops, int n_unary,
    const BinaryOp* binary_ops, int n_binary,
    ErrorMetric metric,
    CompareMode compare)
{
    return vsearch_core(MODE_BATCH, data, n_data, MinK, MaxK, cpu_id, ncpus,
                       const_ops, n_const, unary_ops, n_unary, binary_ops, n_binary,
                       metric, compare, num_to_find);
}
