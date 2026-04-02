// constant_gpu_fp32_hybrid.cu
// Hybrid FP32 GPU search + FP64 CPU verification
// Idea: Use fast FP32 to find candidates, verify with FP64 for true precision
// Code assist: Claude 4.5 Opus
// Compile: nvcc -O3 constant_gpu_fp32_hybrid.cu -o constant_gpu_fp32_hybrid

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

// Candidate threshold: collect expressions with FP32 error below this
// Using a generous threshold to not miss good FP64 candidates
#define CANDIDATE_THRESHOLD 8 //*FLT_EPSILON

// Maximum number of candidates to collect
#define MAX_CANDIDATES (1024 * 1024)  // 1M candidates max

// CODATA 2022 fine structure constant inverse
#define ALPHA_INV_CENTROID 137.035999177
#define ALPHA_INV_SIGMA    2.1e-8
#define N_SIGMA_FILTER     32.0

#define CUDA_CHECK(call) { cudaError_t err = call; if (err != cudaSuccess) { printf("CUDA error: %s\n", cudaGetErrorString(err)); exit(1); } }

// ============================================================================
// Device constants in constant memory (fastest access) - FLOAT version
// ============================================================================

__constant__ float d_const_values[N_CONST] = {
    3.14159265358979323846f, //pi
    2.71828182845904523536f, //e
   -1.0f,                   
    1.61803398874989484820f, //phi
    1.0f,                   
    2.0f,                   
    3.0f,                   
    4.0f,                   
    5.0f,                   
    6.0f,                   
    7.0f,                   
    8.0f,                   
    9.0f                    
};

// Character mappings for output
__constant__ char d_const_chars[N_CONST] = 
    {'0', '1', '2', '3', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w'};
__constant__ char d_unary_chars[N_UNARY] = 
    {'4', '5', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n'};
__constant__ char d_binary_chars[N_BINARY] = 
    {'6', '7', 'x', 'y', 'z'};

// ============================================================================
// Candidate structure - stores info needed to reconstruct expression
// ============================================================================

struct Candidate {
    float fp32_error;              // FP32 relative error (for sorting)
    unsigned long long idx;        // Index within form
    int form_id;                   // Which form
    int K;                         // Code length
};

// Form descriptor
struct FormDesc {
    char ternary[MAX_K + 1];
    int K;
    int radix[MAX_K];
    unsigned long long total;
};

// Verified result for sigma filtering
struct VerifiedResult {
    double computed;
    double sigma_distance;
    int K;
    char amino[MAX_K + 1];
    char mathematica[512];
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
// HYBRID Search kernel - collects candidates, NO early exit
// ============================================================================

__global__ void search_form_kernel_hybrid(
    const char* __restrict__ ternary,
    int K,
    const int* __restrict__ radix,
    unsigned long long total,
    unsigned long long offset,
    float targetX,
    float candidate_threshold,
    Candidate* __restrict__ candidates,
    int* __restrict__ candidate_count,
    int max_candidates,
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
    
    // If this is a candidate (error below threshold), add to buffer
    if (rel_err < candidate_threshold) {
        int slot = atomicAdd(candidate_count, 1);
        if (slot < max_candidates) {
            candidates[slot].fp32_error = rel_err;
            candidates[slot].idx = global_idx;
            candidates[slot].form_id = form_id;
            candidates[slot].K = K;
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
        
        // Compute radix and total combinations
        for (int i = 0; i < K; i++) {
            switch(form.ternary[i]) {
                case '0': form.radix[i] = N_CONST; break;
                case '1': form.radix[i] = N_UNARY; break;
                case '2': form.radix[i] = N_BINARY; break;
            }
            form.total *= form.radix[i];
        }
        
        forms[count++] = form;
    }
    
    return count;
}

// ============================================================================
// Decode result to amino string
// ============================================================================

void decode_result_to_amino(FormDesc* form, unsigned long long idx, char* amino)
{
    const char* const_chars = "0123opqrstuvw";
    const char* unary_chars = "4589abcdefghijklmn";
    const char* binary_chars = "67xyz";
    
    unsigned long long temp = idx;
    for (int i = 0; i < form->K; i++) {
        int slot = temp % form->radix[i];
        temp /= form->radix[i];
        
        switch(form->ternary[i]) {
            case '0': amino[i] = const_chars[slot]; break;
            case '1': amino[i] = unary_chars[slot]; break;
            case '2': amino[i] = binary_chars[slot]; break;
        }
    }
    amino[form->K] = '\0';
}

// ============================================================================
// Convert amino to Mathematica notation
// ============================================================================

const char* const_names[] = {"Pi", "E", "(-1)", "GoldenRatio", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
const char* unary_names[] = {"Log", "Exp", "1/", "Gamma", "Sqrt", "Sqr", "Sin", "ArcSin", "Cos", "ArcCos", "Tan", "ArcTan", "Sinh", "ArcSinh", "Cosh", "ArcCosh", "Tanh", "ArcTanh"};
const char* binary_names[] = {"Plus", "Times", "Subtract", "Divide", "Power"};

void amino_to_mathematica(const char* amino, int K, char* output)
{
    char stack[32][256];
    int sp = 0;
    
    for (int i = 0; i < K; i++) {
        char c = amino[i];
        
        // Constants
        if (c == '0') { strcpy(stack[sp++], "Pi"); }
        else if (c == '1') { strcpy(stack[sp++], "E"); }
        else if (c == '2') { strcpy(stack[sp++], "(-1)"); }
        else if (c == '3') { strcpy(stack[sp++], "GoldenRatio"); }
        else if (c >= 'o' && c <= 'w') { sprintf(stack[sp++], "%d", c - 'o' + 1); }
        
        // Unary functions
        else if (c == '4') { char tmp[256]; strcpy(tmp, stack[--sp]); sprintf(stack[sp++], "Log[%s]", tmp); }
        else if (c == '5') { char tmp[256]; strcpy(tmp, stack[--sp]); sprintf(stack[sp++], "Exp[%s]", tmp); }
        else if (c == '8') { char tmp[256]; strcpy(tmp, stack[--sp]); sprintf(stack[sp++], "(1/%s)", tmp); }
        else if (c == '9') { char tmp[256]; strcpy(tmp, stack[--sp]); sprintf(stack[sp++], "Gamma[%s]", tmp); }
        else if (c == 'a') { char tmp[256]; strcpy(tmp, stack[--sp]); sprintf(stack[sp++], "Sqrt[%s]", tmp); }
        else if (c == 'b') { char tmp[256]; strcpy(tmp, stack[--sp]); sprintf(stack[sp++], "(%s)^2", tmp); }
        else if (c == 'c') { char tmp[256]; strcpy(tmp, stack[--sp]); sprintf(stack[sp++], "Sin[%s]", tmp); }
        else if (c == 'd') { char tmp[256]; strcpy(tmp, stack[--sp]); sprintf(stack[sp++], "ArcSin[%s]", tmp); }
        else if (c == 'e') { char tmp[256]; strcpy(tmp, stack[--sp]); sprintf(stack[sp++], "Cos[%s]", tmp); }
        else if (c == 'f') { char tmp[256]; strcpy(tmp, stack[--sp]); sprintf(stack[sp++], "ArcCos[%s]", tmp); }
        else if (c == 'g') { char tmp[256]; strcpy(tmp, stack[--sp]); sprintf(stack[sp++], "Tan[%s]", tmp); }
        else if (c == 'h') { char tmp[256]; strcpy(tmp, stack[--sp]); sprintf(stack[sp++], "ArcTan[%s]", tmp); }
        else if (c == 'i') { char tmp[256]; strcpy(tmp, stack[--sp]); sprintf(stack[sp++], "Sinh[%s]", tmp); }
        else if (c == 'j') { char tmp[256]; strcpy(tmp, stack[--sp]); sprintf(stack[sp++], "ArcSinh[%s]", tmp); }
        else if (c == 'k') { char tmp[256]; strcpy(tmp, stack[--sp]); sprintf(stack[sp++], "Cosh[%s]", tmp); }
        else if (c == 'l') { char tmp[256]; strcpy(tmp, stack[--sp]); sprintf(stack[sp++], "ArcCosh[%s]", tmp); }
        else if (c == 'm') { char tmp[256]; strcpy(tmp, stack[--sp]); sprintf(stack[sp++], "Tanh[%s]", tmp); }
        else if (c == 'n') { char tmp[256]; strcpy(tmp, stack[--sp]); sprintf(stack[sp++], "ArcTanh[%s]", tmp); }
        
        // Binary operators
        else if (c == '6') { char a[256], b[256]; strcpy(b, stack[--sp]); strcpy(a, stack[--sp]); sprintf(stack[sp++], "(%s+%s)", a, b); }
        else if (c == '7') { char a[256], b[256]; strcpy(b, stack[--sp]); strcpy(a, stack[--sp]); sprintf(stack[sp++], "(%s*%s)", a, b); }
        else if (c == 'x') { char a[256], b[256]; strcpy(b, stack[--sp]); strcpy(a, stack[--sp]); sprintf(stack[sp++], "(%s-%s)", a, b); }
        else if (c == 'y') { char a[256], b[256]; strcpy(b, stack[--sp]); strcpy(a, stack[--sp]); sprintf(stack[sp++], "(%s/%s)", a, b); }
        else if (c == 'z') { char a[256], b[256]; strcpy(b, stack[--sp]); strcpy(a, stack[--sp]); sprintf(stack[sp++], "(%s^%s)", a, b); }
    }
    
    strcpy(output, stack[0]);
}

// ============================================================================
// CPU FP64 evaluation for verification
// ============================================================================

double evaluate_cpu_double(const char* amino, int K)
{
    const double const_values[] = {
        3.14159265358979323846,   // PI
        2.71828182845904523536,   // EULER
       -1.0,                       // NEG
        1.61803398874989484820,   // GOLDENRATIO
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
// Comparison function for sorting candidates by FP32 error
// ============================================================================

int compare_candidates(const void* a, const void* b)
{
    float ea = ((Candidate*)a)->fp32_error;
    float eb = ((Candidate*)b)->fp32_error;
    if (ea < eb) return -1;
    if (ea > eb) return 1;
    return 0;
}

// Comparison function for sorting by absolute sigma distance
int compare_by_sigma(const void* a, const void* b)
{
    double sa = fabs(((VerifiedResult*)a)->sigma_distance);
    double sb = fabs(((VerifiedResult*)b)->sigma_distance);
    if (sa < sb) return -1;
    if (sa > sb) return 1;
    return 0;
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv)
{
    double centroid = ALPHA_INV_CENTROID;
    double sigma = ALPHA_INV_SIGMA;
    double n_sigma = N_SIGMA_FILTER;
    int MaxCodeLength = 9;  
    // Runtime for K=9: 76.458 s on RTX5080; 658.389 s on Tesla T4
    
    // Parse arguments
    if (argc > 1) MaxCodeLength = atoi(argv[1]);
    if (argc > 2) n_sigma = atof(argv[2]);
    
    float targetX = (float) centroid;  
    double abs_threshold = n_sigma * sigma;

    
    printf("=== GPU Alpha Formula Search (HYBRID: FP32 search + FP64 verify) ===\n");
    printf("CODATA centroid: %.12f +/- %.1e\n", centroid, sigma);
    printf("Filter: +/-%.0f sigma = +/-%.3e\n", n_sigma, abs_threshold);
    printf("Max K: %d\n", MaxCodeLength);
    printf("Candidate threshold: %.3e (%d FLT_EPSILON)\n", 
           CANDIDATE_THRESHOLD * FLT_EPSILON, CANDIDATE_THRESHOLD);
    printf("FLT_EPSILON: %.9e\n", FLT_EPSILON);
    printf("DBL_EPSILON: %.17e\n", DBL_EPSILON);
    
    // Get device info
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, 0);
    printf("GPU: %s (%d SMs)\n", prop.name, prop.multiProcessorCount);
    printf("\n");
    
    // Allocate device memory for candidates
    Candidate* d_candidates;
    int* d_candidate_count;
    char* d_ternary;
    int* d_radix;
    
    CUDA_CHECK(cudaMalloc(&d_candidates, MAX_CANDIDATES * sizeof(Candidate)));
    CUDA_CHECK(cudaMalloc(&d_candidate_count, sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_ternary, MAX_K + 1));
    CUDA_CHECK(cudaMalloc(&d_radix, MAX_K * sizeof(int)));
    
    // Initialize candidate count
    int h_candidate_count = 0;
    CUDA_CHECK(cudaMemcpy(d_candidate_count, &h_candidate_count, sizeof(int), cudaMemcpyHostToDevice));
    
    // Allocate host form storage
    int max_forms = 100000;
    FormDesc* all_forms = (FormDesc*)malloc(max_forms * sizeof(FormDesc));
    int* form_offsets = (int*)malloc((MaxCodeLength + 2) * sizeof(int));
    
    // Generate all forms for all K values
    int total_forms = 0;
    form_offsets[0] = 0;
    form_offsets[1] = 0;
    
    printf("Generating valid ternary forms...\n");
    for (int K = 1; K <= MaxCodeLength; K++) {
        int n = generate_valid_forms(K, all_forms + total_forms, max_forms - total_forms);
        total_forms += n;
        form_offsets[K + 1] = total_forms;
        
        unsigned long long total_for_K = 0;
        for (int i = form_offsets[K]; i < form_offsets[K + 1]; i++) {
            total_for_K += all_forms[i].total;
        }
        printf("K=%2d: %5d forms, %15llu combinations\n", K, n, total_for_K);
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
    
    printf("=== Phase 1: FP32 GPU Search (collecting candidates) ===\n");
    
    // Main search loop - NO early exit!
    for (int K = 1; K <= MaxCodeLength; K++) {
        
        for (int form_id = form_offsets[K]; form_id < form_offsets[K + 1]; form_id++) {
            
            FormDesc* form = &all_forms[form_id];
            
            // Upload form to device
            CUDA_CHECK(cudaMemcpy(d_ternary, form->ternary, K + 1, cudaMemcpyHostToDevice));
            CUDA_CHECK(cudaMemcpy(d_radix, form->radix, K * sizeof(int), cudaMemcpyHostToDevice));
            
            // Process in chunks
            for (unsigned long long offset = 0; offset < form->total; offset += chunk_size) {
                unsigned long long count = form->total - offset;
                if (count > chunk_size) count = chunk_size;
                
                int blocks = (count + threadsPerBlock - 1) / threadsPerBlock;
                
                search_form_kernel_hybrid<<<blocks, threadsPerBlock>>>(
                    d_ternary, K, d_radix, count, offset,
                    targetX, CANDIDATE_THRESHOLD * FLT_EPSILON,
                    d_candidates, d_candidate_count, MAX_CANDIDATES, form_id
                );
            }
            
            total_evaluated += form->total;
        }
        
        // Progress update after each K
        CUDA_CHECK(cudaMemcpy(&h_candidate_count, d_candidate_count, sizeof(int), cudaMemcpyDeviceToHost));
        printf("K=%2d complete: %d candidates so far\n", K, h_candidate_count);
    }
    
    // Stop GPU timing
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    
    float gpu_milliseconds = 0;
    cudaEventElapsedTime(&gpu_milliseconds, start, stop);
    
    // Get final candidate count
    CUDA_CHECK(cudaMemcpy(&h_candidate_count, d_candidate_count, sizeof(int), cudaMemcpyDeviceToHost));
    
    printf("\n=== Phase 1 Complete ===\n");
    printf("Total evaluations: %llu\n", total_evaluated);
    printf("GPU time: %.3f s\n", gpu_milliseconds / 1000.0);
    printf("Throughput: %.2f M evals/sec\n", total_evaluated / gpu_milliseconds / 1000.0);
    printf("Candidates found: %d\n", h_candidate_count);
    
    if (h_candidate_count >= MAX_CANDIDATES) {
        printf("WARNING: Candidate buffer overflow! Increase MAX_CANDIDATES or threshold.\n");
        h_candidate_count = MAX_CANDIDATES;
    }
    
    // Copy candidates to host
    Candidate* h_candidates = (Candidate*)malloc(h_candidate_count * sizeof(Candidate));
    CUDA_CHECK(cudaMemcpy(h_candidates, d_candidates, h_candidate_count * sizeof(Candidate), cudaMemcpyDeviceToHost));
    
    printf("\n=== Phase 2: FP64 CPU Verification & Sigma Filtering ===\n");
    
    // Allocate storage for verified results
    VerifiedResult* results = (VerifiedResult*)malloc(h_candidate_count * sizeof(VerifiedResult));
    int n_verified = 0;
    
    for (int i = 0; i < h_candidate_count; i++) {
        Candidate* c = &h_candidates[i];
        
        // Decode to amino string
        char amino[MAX_K + 1];
        decode_result_to_amino(&all_forms[c->form_id], c->idx, amino);
        
        // Evaluate in FP64
        double computed = evaluate_cpu_double(amino, c->K);
        if (isnan(computed) || isinf(computed)) continue;
        
        double abs_err = fabs(computed - centroid);
        double sigma_dist = (computed - centroid) / sigma;
        
        // Filter: within +/- n_sigma
        if (abs_err <= abs_threshold) {
            results[n_verified].computed = computed;
            results[n_verified].sigma_distance = sigma_dist;
            results[n_verified].K = c->K;
            strcpy(results[n_verified].amino, amino);
            amino_to_mathematica(amino, c->K, results[n_verified].mathematica);
            n_verified++;
        }
    }
    
    printf("Formulas within +/-%.0f sigma: %d\n", n_sigma, n_verified);
    
    // Sort by absolute sigma distance
    qsort(results, n_verified, sizeof(VerifiedResult), compare_by_sigma);
    
    // Print detailed table
    printf("\n%-4s %-2s %-14s %-10s %s\n", "Rank", "K", "Computed", "Sigma", "Amino");
    printf("--------------------------------------------------\n");
    for (int i = 0; i < n_verified; i++) {
        printf("%4d %2d %.12f %+8.2f %s\n", 
            i+1, results[i].K, results[i].computed, results[i].sigma_distance, results[i].amino);
    }
    
    
    // Write to file
    FILE* f = fopen("alpha_formulas.wl", "w");
    if (f) {
        fprintf(f, "(* Alpha^(-1) formula candidates from GPU exhaustive search *)\n");
        fprintf(f, "(* CODATA 2022: %.12f +/- %.1e *)\n", centroid, sigma);
        fprintf(f, "(* Filter: +/-%.0f sigma, Max K: %d *)\n\n", n_sigma, MaxCodeLength);
        fprintf(f, "{\n");
        for (int i = 0; i < n_verified; i++) {
            fprintf(f, "  %s", results[i].mathematica);
            if (i < n_verified - 1) fprintf(f, ",");
            fprintf(f, "  (* K=%d, sigma=%+.2f *)\n", results[i].K, results[i].sigma_distance);
        }
        fprintf(f, "}\n");
        fclose(f);
        printf("\nFormulas saved to: alpha_formulas.wl\n");
    }
    
    free(results);
    
    printf("\n=== SUMMARY ===\n");
    printf("CODATA target:   %.12f +/- %.1e\n", centroid, sigma);
    printf("GPU time:        %.3f s\n", gpu_milliseconds / 1000.0);
    printf("GPU throughput:  %.2f M evals/sec\n", total_evaluated / gpu_milliseconds / 1000.0);
    printf("FP32 candidates: %d\n", h_candidate_count);
    printf("Verified hits:   %d (within +/-%.0f sigma)\n", n_verified, n_sigma);
    
    // Cleanup
    free(h_candidates);
    free(all_forms);
    free(form_offsets);
    cudaFree(d_candidates);
    cudaFree(d_candidate_count);
    cudaFree(d_ternary);
    cudaFree(d_radix);
    
    cudaEventDestroy(start);
    cudaEventDestroy(stop);
    
    return 0;
}
