// constant_gpu_fp32.cu
// FP32 (single precision) version for faster "consumer" searches
// Compile: nvcc -O3 -arch=sm_75 constant_gpu_fp32.cu -o constant_gpu_fp32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cuda_runtime.h>
#include <float.h>
#include <math.h>
#include <stdint.h>

#define STACKSIZE 16
#define MAX_K 12
#define N_CONST  13
#define N_UNARY  18
#define N_BINARY  5

#define EPS_MAX 16
#define CUDA_CHECK(call) { cudaError_t err = call; if (err != cudaSuccess) { printf("CUDA error: %s\n", cudaGetErrorString(err)); exit(1); } }

// ============================================================================
// Device constants in constant memory (fastest access) - FLOAT version
// ============================================================================

__constant__ float d_const_values[N_CONST] = {
    3.14159265358979323846f,   // 0: PI
    2.71828182845904523536f,   // 1: EULER
   -1.0f,                       // 2: NEG
    1.61803398874989484820f,   // 3: GOLDENRATIO
    1.0f,                       // 4: ONE
    2.0f,                       // 5: TWO
    3.0f,                       // 6: THREE
    4.0f,                       // 7: FOUR
    5.0f,                       // 8: FIVE
    6.0f,                       // 9: SIX
    7.0f,                       // 10: SEVEN
    8.0f,                       // 11: EIGHT
    9.0f                        // 12: NINE
};

// Character mappings for output
__constant__ char d_const_chars[N_CONST] = 
    {'0', '1', '2', '3', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w'};
__constant__ char d_unary_chars[N_UNARY] = 
    {'4', '5', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n'};
__constant__ char d_binary_chars[N_BINARY] = 
    {'6', '7', 'x', 'y', 'z'};

// ============================================================================
// Result structure - FLOAT version
// ============================================================================

struct SearchResult {
    float error;
    unsigned long long idx;      // Index within form
    int form_id;                 // Which form found it
    int K;                       // Code length
};

// Form descriptor
struct FormDesc {
    char ternary[MAX_K + 1];
    int K;
    int radix[MAX_K];
    unsigned long long total;
};

// ============================================================================
// Host-side syntax check for ternary forms
// ============================================================================

__host__ int checkSyntax3_host(const char* ternary, int length)
{
    int stack = 0;
    for (int i = 0; i < length; i++) {
        switch(ternary[i]) {
            case '0': stack++; break;
            case '1': if (stack < 1) return 0; break;
            case '2': if (stack < 2) return 0; stack--; break;
        }
    }
    return (stack == 1);
}

// ============================================================================
// Direct evaluation kernel - FLOAT version
// ============================================================================

__device__ __forceinline__ float apply_unary(int op, float x)
{
    switch(op) {
        case 0:  return logf(x);
        case 1:  return expf(x);
        case 2:  return 1.0f / x;
        case 3:  return tgammaf(x);
        case 4:  return sqrtf(x);
        case 5:  return x * x;
        case 6:  return sinf(x);
        case 7:  return asinf(x);
        case 8:  return cosf(x);
        case 9:  return acosf(x);
        case 10: return tanf(x);
        case 11: return atanf(x);
        case 12: return sinhf(x);
        case 13: return asinhf(x);
        case 14: return coshf(x);
        case 15: return acoshf(x);
        case 16: return tanhf(x);
        case 17: return atanhf(x);
        default: return nanf("");
    }
}

__device__ __forceinline__ float apply_binary(int op, float a, float b)
{
    switch(op) {
        case 0: return a + b;
        case 1: return a * b;
        case 2: return a - b;
        case 3: return a / b;
        case 4: return powf(a, b);
        default: return nanf("");
    }
}

__device__ float evaluate_form_direct(
    const char* __restrict__ ternary,
    const int* __restrict__ slots,
    int K
)
{
    float stack[STACKSIZE];
    int sp = 0;
    
    #pragma unroll 4
    for (int i = 0; i < K; i++) {
        char t = ternary[i];
        int slot = slots[i];
        
        if (t == '0') {
            stack[sp++] = d_const_values[slot];
        }
        else if (t == '1') {
            stack[sp-1] = apply_unary(slot, stack[sp-1]);
        }
        else { // t == '2'
            sp--;
            stack[sp-1] = apply_binary(slot, stack[sp-1], stack[sp]);
        }
    }
    
    return stack[0];
}

// ============================================================================
// Build amino string from form + slots (for output only)
// ============================================================================

__device__ void build_amino_from_slots(
    const char* ternary,
    const int* slots,
    int K,
    char* amino
)
{
    for (int i = 0; i < K; i++) {
        switch(ternary[i]) {
            case '0': amino[i] = d_const_chars[slots[i]]; break;
            case '1': amino[i] = d_unary_chars[slots[i]]; break;
            case '2': amino[i] = d_binary_chars[slots[i]]; break;
        }
    }
    amino[K] = '\0';
}

// ============================================================================
// Main search kernel - FLOAT version with 32-bit atomics
// ============================================================================

__global__ void search_form_kernel(
    const char* __restrict__ ternary,
    int K,
    const int* __restrict__ radix,
    unsigned long long total,
    unsigned long long offset,
    float targetX,
    float Delta_z,
    SearchResult* __restrict__ best_result,
    int* __restrict__ found_flag,
    int form_id
)
{
    // Load radix into shared memory
    __shared__ int s_radix[MAX_K];
    __shared__ char s_ternary[MAX_K + 1];
    
    if (threadIdx.x < K) {
        s_radix[threadIdx.x] = radix[threadIdx.x];
        s_ternary[threadIdx.x] = ternary[threadIdx.x];
    }
    if (threadIdx.x == 0) {
        s_ternary[K] = '\0';
    }
    __syncthreads();
    
    unsigned long long idx = blockIdx.x * (unsigned long long)blockDim.x + threadIdx.x;
    if (idx >= total) return;
    
    // Early exit if already found
    if (*found_flag) return;
    
    unsigned long long global_idx = offset + idx;
    
    // Decode mixed-radix index into slot values
    int slots[MAX_K];
    unsigned long long temp = global_idx;
    
    #pragma unroll
    for (int i = 0; i < MAX_K; i++) {
        if (i >= K) break;
        slots[i] = temp % s_radix[i];
        temp /= s_radix[i];
    }
    
    // Evaluate directly
    float computedX = evaluate_form_direct(s_ternary, slots, K);
    
    // Skip NaN
    if (isnan(computedX)) return;
    
    // Compute relative error
    float rel_err;
    if (targetX == 0.0f)
        rel_err = fabsf(computedX);
    else
        rel_err = fabsf(computedX / targetX - 1.0f);
    
    // Load current best for comparison
    float current_best = best_result->error;
    
    // Only attempt atomic if we might improve
    if (rel_err < current_best) {
        // Atomic compare-and-swap for float (32-bit)
        int* addr = (int*)&best_result->error;
        int old = __float_as_int(current_best);
        int assumed;
        
        do {
            assumed = old;
            if (__int_as_float(assumed) <= rel_err) break;
            old = atomicCAS(addr, assumed, __float_as_int(rel_err));
        } while (assumed != old);
        
        // If we successfully updated, store our result info
        if (__int_as_float(old) > rel_err) {
            best_result->idx = global_idx;
            best_result->form_id = form_id;
            best_result->K = K;
            
            // Check for "exact" match (FP32 epsilon)
            if (rel_err <= EPS_MAX * FLT_EPSILON) {
                atomicExch(found_flag, 1);
            }
            
            // Also check compression criterion
            if (Delta_z > 0.0f) {
                float abs_err = fabsf(computedX - targetX);
                if (abs_err <= 2.0f * Delta_z) {
                    atomicExch(found_flag, 1);
                }
            }
        }
    }
}

// ============================================================================
// Host: Generate all valid ternary forms for given K
// ============================================================================

int generate_valid_forms(int K, FormDesc* forms, int max_forms)
{
    int count = 0;
    unsigned long long max_ternary = 1;
    for (int i = 0; i < K; i++) max_ternary *= 3;
    
    for (unsigned long long t = 0; t < max_ternary && count < max_forms; t++) {
        FormDesc form;
        form.K = K;
        form.total = 1;
        
        // Convert to ternary string
        unsigned long long temp = t;
        for (int i = 0; i < K; i++) {
            form.ternary[i] = '0' + (temp % 3);
            temp /= 3;
        }
        form.ternary[K] = '\0';
        
        // Check syntax
        if (!checkSyntax3_host(form.ternary, K)) continue;
        
        // Compute radix and total
        for (int i = 0; i < K; i++) {
            switch(form.ternary[i]) {
                case '0': form.radix[i] = N_CONST;  break;
                case '1': form.radix[i] = N_UNARY;  break;
                case '2': form.radix[i] = N_BINARY; break;
            }
            form.total *= form.radix[i];
        }
        
        forms[count++] = form;
    }
    
    return count;
}

// ============================================================================
// Host: Decode result back to amino string
// ============================================================================

void decode_result_to_amino(
    const FormDesc* form,
    unsigned long long idx,
    char* amino
)
{
    static const char const_chars[N_CONST] = 
        {'0', '1', '2', '3', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w'};
    static const char unary_chars[N_UNARY] = 
        {'4', '5', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n'};
    static const char binary_chars[N_BINARY] = 
        {'6', '7', 'x', 'y', 'z'};
    
    int K = form->K;
    unsigned long long temp = idx;
    
    for (int i = 0; i < K; i++) {
        int slot = temp % form->radix[i];
        temp /= form->radix[i];
        
        switch(form->ternary[i]) {
            case '0': amino[i] = const_chars[slot]; break;
            case '1': amino[i] = unary_chars[slot]; break;
            case '2': amino[i] = binary_chars[slot]; break;
        }
    }
    amino[K] = '\0';
}

// ============================================================================
// Host: Convert amino to Mathematica-readable format
// ============================================================================

void amino_to_mathematica(const char* amino, int K, char* output)
{
    output[0] = '\0';
    
    for (int i = 0; i < K; i++) {
        if (i > 0) strcat(output, ", ");
        
        switch(amino[i]) {
            case '0': strcat(output, "PI"); break;
            case '1': strcat(output, "EULER"); break;
            case '2': strcat(output, "NEG"); break;
            case '3': strcat(output, "GOLDENRATIO"); break;
            case '4': strcat(output, "LOG"); break;
            case '5': strcat(output, "EXP"); break;
            case '6': strcat(output, "PLUS"); break;
            case '7': strcat(output, "TIMES"); break;
            case '8': strcat(output, "INV"); break;
            case '9': strcat(output, "GAMMA"); break;
            case 'a': strcat(output, "SQRT"); break;
            case 'b': strcat(output, "SQR"); break;
            case 'c': strcat(output, "SIN"); break;
            case 'd': strcat(output, "ARCSIN"); break;
            case 'e': strcat(output, "COS"); break;
            case 'f': strcat(output, "ARCCOS"); break;
            case 'g': strcat(output, "TAN"); break;
            case 'h': strcat(output, "ARCTAN"); break;
            case 'i': strcat(output, "SINH"); break;
            case 'j': strcat(output, "ARCSINH"); break;
            case 'k': strcat(output, "COSH"); break;
            case 'l': strcat(output, "ARCCOSH"); break;
            case 'm': strcat(output, "TANH"); break;
            case 'n': strcat(output, "ARCTANH"); break;
            case 'o': strcat(output, "ONE"); break;
            case 'p': strcat(output, "TWO"); break;
            case 'q': strcat(output, "THREE"); break;
            case 'r': strcat(output, "FOUR"); break;
            case 's': strcat(output, "FIVE"); break;
            case 't': strcat(output, "SIX"); break;
            case 'u': strcat(output, "SEVEN"); break;
            case 'v': strcat(output, "EIGHT"); break;
            case 'w': strcat(output, "NINE"); break;
            case 'x': strcat(output, "SUBTRACT"); break;
            case 'y': strcat(output, "DIVIDE"); break;
            case 'z': strcat(output, "POWER"); break;
        }
    }
}

// ============================================================================
// Host: Evaluate on CPU for verification (using DOUBLE for accurate check!)
// ============================================================================

double evaluate_cpu_double(const char* amino, int K)
{
    static const double const_values[N_CONST] = {
        3.14159265358979323846264338327950288,
        2.71828182845904523536028747135266250,
       -1.0,
        1.61803398874989484820458683436563812,
        1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0
    };
    
    double stack[STACKSIZE];
    int sp = 0;
    
    for (int i = 0; i < K; i++) {
        char c = amino[i];
        switch(c) {
            case '0': stack[sp++] = const_values[0]; break;
            case '1': stack[sp++] = const_values[1]; break;
            case '2': stack[sp++] = const_values[2]; break;
            case '3': stack[sp++] = const_values[3]; break;
            case 'o': stack[sp++] = const_values[4]; break;
            case 'p': stack[sp++] = const_values[5]; break;
            case 'q': stack[sp++] = const_values[6]; break;
            case 'r': stack[sp++] = const_values[7]; break;
            case 's': stack[sp++] = const_values[8]; break;
            case 't': stack[sp++] = const_values[9]; break;
            case 'u': stack[sp++] = const_values[10]; break;
            case 'v': stack[sp++] = const_values[11]; break;
            case 'w': stack[sp++] = const_values[12]; break;
            case '4': stack[sp-1] = log(stack[sp-1]); break;
            case '5': stack[sp-1] = exp(stack[sp-1]); break;
            case '8': stack[sp-1] = 1.0 / stack[sp-1]; break;
            case '9': stack[sp-1] = tgamma(stack[sp-1]); break;
            case 'a': stack[sp-1] = sqrt(stack[sp-1]); break;
            case 'b': stack[sp-1] = stack[sp-1] * stack[sp-1]; break;
            case 'c': stack[sp-1] = sin(stack[sp-1]); break;
            case 'd': stack[sp-1] = asin(stack[sp-1]); break;
            case 'e': stack[sp-1] = cos(stack[sp-1]); break;
            case 'f': stack[sp-1] = acos(stack[sp-1]); break;
            case 'g': stack[sp-1] = tan(stack[sp-1]); break;
            case 'h': stack[sp-1] = atan(stack[sp-1]); break;
            case 'i': stack[sp-1] = sinh(stack[sp-1]); break;
            case 'j': stack[sp-1] = asinh(stack[sp-1]); break;
            case 'k': stack[sp-1] = cosh(stack[sp-1]); break;
            case 'l': stack[sp-1] = acosh(stack[sp-1]); break;
            case 'm': stack[sp-1] = tanh(stack[sp-1]); break;
            case 'n': stack[sp-1] = atanh(stack[sp-1]); break;
            case '6': sp--; stack[sp-1] = stack[sp-1] + stack[sp]; break;
            case '7': sp--; stack[sp-1] = stack[sp-1] * stack[sp]; break;
            case 'x': sp--; stack[sp-1] = stack[sp-1] - stack[sp]; break;
            case 'y': sp--; stack[sp-1] = stack[sp-1] / stack[sp]; break;
            case 'z': sp--; stack[sp-1] = pow(stack[sp-1], stack[sp]); break;
        }
    }
    return stack[0];
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv)
{
    float targetX = 777.0f;
    float Delta_z = 0.0f;
    int MaxCodeLength = 7;
    
    // Parse arguments
    if (argc > 1) targetX = atof(argv[1]);
    if (argc > 2) MaxCodeLength = atoi(argv[2]);
    if (argc > 3) Delta_z = atof(argv[3]);
    
    printf("=== GPU Constant Recognition (FP32 / Single Precision) ===\n");
    printf("Target: %.9g\n", targetX);
    printf("Delta_z: %.9g\n", Delta_z);
    printf("Max K: %d\n", MaxCodeLength);
    printf("FLT_EPSILON: %.9e (vs DBL_EPSILON: %.17e)\n", FLT_EPSILON, DBL_EPSILON);
    
    // Get device info
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, 0);
    printf("GPU: %s (%d SMs, %d cores/SM)\n", 
           prop.name, prop.multiProcessorCount, 
           prop.maxThreadsPerMultiProcessor);
    printf("\n");
    
    // Allocate device memory
    SearchResult* d_best;
    int* d_found_flag;
    char* d_ternary;
    int* d_radix;
    
    CUDA_CHECK(cudaMalloc(&d_best, sizeof(SearchResult)));
    CUDA_CHECK(cudaMalloc(&d_found_flag, sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_ternary, MAX_K + 1));
    CUDA_CHECK(cudaMalloc(&d_radix, MAX_K * sizeof(int)));
    
    // Initialize result
    SearchResult h_best;
    h_best.error = FLT_MAX;
    h_best.idx = 0;
    h_best.form_id = -1;
    h_best.K = 0;
    int h_found = 0;
    
    CUDA_CHECK(cudaMemcpy(d_best, &h_best, sizeof(SearchResult), cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_found_flag, &h_found, sizeof(int), cudaMemcpyHostToDevice));
    
    // Allocate host form storage
    int max_forms = 100000;
    FormDesc* all_forms = (FormDesc*)malloc(max_forms * sizeof(FormDesc));
    int* form_offsets = (int*)malloc((MaxCodeLength + 2) * sizeof(int));
    
    // Generate all forms for all K values
    int total_forms = 0;
    form_offsets[0] = 0;
    form_offsets[1] = 0;
    
    for (int K = 1; K <= MaxCodeLength; K++) {
        int n = generate_valid_forms(K, all_forms + total_forms, max_forms - total_forms);
        total_forms += n;
        form_offsets[K + 1] = total_forms;
        printf("K=%d: %d valid forms (total combinations: ", K, n);
        
        unsigned long long total_for_K = 0;
        for (int i = form_offsets[K]; i < form_offsets[K + 1]; i++) {
            total_for_K += all_forms[i].total;
        }
        printf("%llu)\n", total_for_K);
    }
    printf("Total forms: %d\n\n", total_forms);
    
    // Timing
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start);
    
    // Search parameters
    int threadsPerBlock = 256;
    unsigned long long chunk_size = 1ULL << 26;  // 64M per kernel launch
    unsigned long long total_evaluated = 0;
    
    // Main search loop
    for (int K = 1; K <= MaxCodeLength && !h_found; K++) {
        
        for (int form_id = form_offsets[K]; form_id < form_offsets[K + 1] && !h_found; form_id++) {
            
            FormDesc* form = &all_forms[form_id];
            
            // Upload form to device
            CUDA_CHECK(cudaMemcpy(d_ternary, form->ternary, K + 1, cudaMemcpyHostToDevice));
            CUDA_CHECK(cudaMemcpy(d_radix, form->radix, K * sizeof(int), cudaMemcpyHostToDevice));
            
            // Process in chunks
            for (unsigned long long offset = 0; offset < form->total && !h_found; offset += chunk_size) {
                unsigned long long count = form->total - offset;
                if (count > chunk_size) count = chunk_size;
                
                int blocks = (count + threadsPerBlock - 1) / threadsPerBlock;
                
                search_form_kernel<<<blocks, threadsPerBlock>>>(
                    d_ternary, K, d_radix, count, offset,
                    targetX, Delta_z, d_best, d_found_flag, form_id
                );
                
                // Check for completion periodically
                CUDA_CHECK(cudaMemcpy(&h_found, d_found_flag, sizeof(int), cudaMemcpyDeviceToHost));
            }
            
            total_evaluated += form->total;
        }
        
        // Progress update
        if (!h_found) {
            CUDA_CHECK(cudaMemcpy(&h_best, d_best, sizeof(SearchResult), cudaMemcpyDeviceToHost));
            printf("K=%d complete, best error so far: %.6e\n", K, h_best.error);
        }
    }
    
    // Stop timing
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    
    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, start, stop);
    
    // Get final result
    CUDA_CHECK(cudaMemcpy(&h_best, d_best, sizeof(SearchResult), cudaMemcpyDeviceToHost));
    
    // Decode result
    char amino[MAX_K + 1] = {0};
    char mathematica[1024] = {0};
    double computed_value_double = 0.0;
    
    if (h_best.form_id >= 0 && h_best.form_id < total_forms) {
        FormDesc* winning_form = &all_forms[h_best.form_id];
        decode_result_to_amino(winning_form, h_best.idx, amino);
        amino_to_mathematica(amino, h_best.K, mathematica);
        computed_value_double = evaluate_cpu_double(amino, h_best.K);  // Use double for verification!
    }
    
    // Print results
    printf("\n");
    printf("=== Results ===\n");
    printf("Status: %s\n", h_found ? "SUCCESS (exact match within FP32 precision)" : "BEST FOUND");
    printf("Target:         %.17g\n", (double)targetX);
    printf("Computed (FP64): %.17g\n", computed_value_double);
    printf("FP32 rel error: %.9e\n", h_best.error);
    printf("FP64 rel error: %.17e\n", fabs(computed_value_double / (double)targetX - 1.0));
    printf("Error in eps:   %.6f × FLT_EPSILON\n", h_best.error / FLT_EPSILON);
    printf("\n");
    printf("Code length K: %d\n", h_best.K);
    printf("Short code:    %s\n", amino);
    printf("RPN code:      {%s}\n", mathematica);
    printf("\n");
    printf("=== Performance ===\n");
    printf("Total evaluations: %llu\n", total_evaluated);
    printf("Time: %.3f ms (%.3f s)\n", milliseconds, milliseconds / 1000.0);
    printf("Throughput: %.2f M evals/sec\n", total_evaluated / milliseconds / 1000.0);
    printf("Throughput: %.2f G evals/sec\n", total_evaluated / milliseconds / 1000000.0);
    
    // Cleanup
    free(all_forms);
    free(form_offsets);
    cudaFree(d_best);
    cudaFree(d_found_flag);
    cudaFree(d_ternary);
    cudaFree(d_radix);
    
    cudaEventDestroy(start);
    cudaEventDestroy(stop);
    
    return h_found ? 0 : 1;
}
