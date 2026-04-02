// constant_gpu_hpc_complex.cu
// HPC-ready GPU implementation - EXHAUSTIVE COMPLEX SEARCH
// Features:
// 1. Interrupt Checkpointing (Atomic Save/Resume for distributed workers)
// 2. Thrust Complex Math (thrust::complex<double>)
// 3. Multi-Node Distribution (MPI / SLURM scaling via Worker IDs)
// 4. Exhaustive Candidate Buffering (collects ALL matches within Error bounds)
// Compile: nvcc -O3 -arch=sm_75 constant_gpu_hpc_complex.cu -o constant_gpu_hpc_complex

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cuda_runtime.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <thrust/complex.h>
#ifdef _WIN32
#include <io.h>
#define access _access
#else
#include <unistd.h>
#endif

#define STACKSIZE 16
#define MAX_K 12
#define N_CONST  14  // Added Imaginary Unit
#define N_UNARY  18
#define N_BINARY  5

// Maximum candidates to collect in a single chunk
#define MAX_CANDIDATES (4 * 1024 * 1024)  // 4M buffer

// We use thrust::complex<double> which overloads all std mathematical operators!
typedef thrust::complex<double> ComplexT;

#define CUDA_CHECK(call) { cudaError_t err = call; if (err != cudaSuccess) { printf("CUDA error: %s\n", cudaGetErrorString(err)); exit(1); } }

// ============================================================================
// Device constants in constant memory 
// ============================================================================

__constant__ ComplexT d_const_values[N_CONST];

// Character mappings for output -> Added 'I' for imaginary
__constant__ char d_const_chars[N_CONST] = 
    {'0', '1', '2', '3', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'I'};
__constant__ char d_unary_chars[N_UNARY] = 
    {'4', '5', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n'};
__constant__ char d_binary_chars[N_BINARY] = 
    {'6', '7', 'x', 'y', 'z'};

// ============================================================================
// Data Structures
// ============================================================================

struct Candidate {
    double rel_err;
    unsigned long long idx;      // Index within form
    int form_id;                 // Which form found it
    int K;                       // Code length
};

struct FormDesc {
    char ternary[MAX_K + 1];
    int K;
    int radix[MAX_K];
    unsigned long long total;
};

// HPC Checkpoint State
struct HPCState {
    int start_K;
    int start_form_id;
    unsigned long long start_offset;
};

// ============================================================================
// Direct evaluation kernel - Complex version
// ============================================================================

__device__ __forceinline__ ComplexT apply_unary(int op, ComplexT x)
{
    switch(op) {
        case 0:  return thrust::log(x);
        case 1:  return thrust::exp(x);
        case 2:  return ComplexT(1.0, 0.0) / x;
        case 3:  if (fabs(x.imag()) < 1e-9) { return ComplexT(tgamma(x.real()), 0.0); } else { return ComplexT(0.0, 0.0); } 
        case 4:  return thrust::sqrt(x);
        case 5:  return x * x;
        case 6:  return thrust::sin(x);
        case 7:  return thrust::asin(x);
        case 8:  return thrust::cos(x);
        case 9:  return thrust::acos(x);
        case 10: return thrust::tan(x);
        case 11: return thrust::atan(x);
        case 12: return thrust::sinh(x);
        case 13: return thrust::asinh(x);
        case 14: return thrust::cosh(x);
        case 15: return thrust::acosh(x);
        case 16: return thrust::tanh(x);
        case 17: return thrust::atanh(x);
        default: return ComplexT(0.0, 0.0);
    }
}

__device__ __forceinline__ ComplexT apply_binary(int op, ComplexT a, ComplexT b)
{
    switch(op) {
        case 0: return a + b;
        case 1: return a * b;
        case 2: return a - b;
        case 3: return a / b;
        case 4: return thrust::pow(a, b);
        default: return ComplexT(NAN, NAN);
    }
}

__device__ ComplexT evaluate_form_direct(
    const char* __restrict__ ternary,
    const int* __restrict__ slots,
    int K
)
{
    ComplexT stack[STACKSIZE];
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
// Main search EXHAUSTIVE kernel
// ============================================================================

__global__ void search_form_kernel_exhaustive(
    const char* __restrict__ ternary,
    int K,
    const int* __restrict__ radix,
    unsigned long long total,
    unsigned long long offset,
    ComplexT targetC,
    double targetMagnitude,
    double tolerance,
    Candidate* __restrict__ buffer,
    int* __restrict__ buffer_count,
    int max_candidates,
    int form_id
)
{
    __shared__ int s_radix[MAX_K];
    __shared__ char s_ternary[MAX_K + 1];
    
    if (threadIdx.x < K) {
        s_radix[threadIdx.x] = radix[threadIdx.x];
        s_ternary[threadIdx.x] = ternary[threadIdx.x];
    }
    if (threadIdx.x == 0) s_ternary[K] = '\0';
    __syncthreads();
    
    unsigned long long idx = blockIdx.x * (unsigned long long)blockDim.x + threadIdx.x;
    if (idx >= total) return;
    
    unsigned long long global_idx = offset + idx;
    
    int slots[MAX_K];
    unsigned long long temp = global_idx;
    
    #pragma unroll
    for (int i = 0; i < MAX_K; i++) {
        if (i >= K) break;
        slots[i] = temp % s_radix[i];
        temp /= s_radix[i];
    }
    
    ComplexT computedC = evaluate_form_direct(s_ternary, slots, K);
    if (isnan(computedC.real()) || isnan(computedC.imag())) return;
    
    // Relative error in complex plane
    double rel_err;
    if (targetMagnitude == 0.0)
        rel_err = thrust::abs(computedC);
    else
        rel_err = thrust::abs(computedC / targetC - ComplexT(1.0, 0.0));
    
    // EXHAUSTIVE COLLECTION within tolerance
    if (rel_err <= tolerance) {
        int pos = atomicAdd(buffer_count, 1);
        if (pos < max_candidates) {
            buffer[pos].rel_err = rel_err;
            buffer[pos].idx = global_idx;
            buffer[pos].form_id = form_id;
            buffer[pos].K = K;
        }
    }
}

// ============================================================================
// HPC Checkpointing Atomic File IO
// ============================================================================

bool file_exists(const char *filename) {
    if (FILE *file = fopen(filename, "r")) {
        fclose(file);
        return true;
    }
    return false;
}

void save_hpc_state(const char* fname, HPCState s) {
    char temp_name[512];
    snprintf(temp_name, sizeof(temp_name), "%s.tmp", fname);
    
    FILE* f = fopen(temp_name, "w");
    if (f) {
        fprintf(f, "%d %d %llu\n", s.start_K, s.start_form_id, s.start_offset);
        // Force flush if necessary
        fflush(f);
        fclose(f);
        
        // Atomic rename (overwrites if target exists on POSIX, on Windows sometimes needs unlink first)
        remove(fname); 
        rename(temp_name, fname);
    }
}

bool load_hpc_state(const char* fname, HPCState* s) {
    if(!file_exists(fname)) return false;
    FILE* f = fopen(fname, "r");
    if (f) {
        if (fscanf(f, "%d %d %llu", &s->start_K, &s->start_form_id, &s->start_offset) == 3) {
            fclose(f);
            return true;
        }
        fclose(f);
    }
    return false;
}

// Decode logic for writing log dump
void decode_result_to_amino(const FormDesc* form, unsigned long long idx, char* amino) {
    const char const_chars[N_CONST] = {'0', '1', '2', '3', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'I'};
    const char unary_chars[N_UNARY] = {'4', '5', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n'};
    const char binary_chars[N_BINARY] = {'6', '7', 'x', 'y', 'z'};
    
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

void flush_candidates_to_disk(const char* log_file, Candidate* h_candidates, int count, const FormDesc* all_forms) {
    FILE* f = fopen(log_file, "a");
    if(!f) return;
    
    char amino[MAX_K + 1];
    for (int i=0; i<count; i++) {
        if (i >= MAX_CANDIDATES) {
            printf("WARNING: GPU Candidate Buffer Overflow. Candidates silently dropped!\n");
            break;
        }
        Candidate c = h_candidates[i];
        decode_result_to_amino(&all_forms[c.form_id], c.idx, amino);
        fprintf(f, "%d\t%.17g\t%s\n", c.K, c.rel_err, amino);
    }
    fclose(f);
}

// Validation Logic
int checkSyntax3_host(const char* ternary, int length) {
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

int generate_valid_forms(int K, FormDesc* forms, int max_forms) {
    int count = 0;
    unsigned long long max_ternary = 1;
    for (int i = 0; i < K; i++) max_ternary *= 3;
    for (unsigned long long t = 0; t < max_ternary && count < max_forms; t++) {
        FormDesc form;
        form.K = K; form.total = 1;
        unsigned long long temp = t;
        for (int i = 0; i < K; i++) { form.ternary[i] = '0' + (temp % 3); temp /= 3; }
        form.ternary[K] = '\0';
        if (!checkSyntax3_host(form.ternary, K)) continue;
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


int main(int argc, char** argv)
{
    // Defaults
    double tgReal = 137.035999177; 
    double tgImag = 0.0;
    int MaxCodeLength = 7;
    double tolerance = 1e-6;   // Default 10^-6, user specifies this
    int worker_id = 0;
    int total_workers = 1;
    
    // Parse
    if (argc > 1) tgReal = atof(argv[1]);
    if (argc > 2) tgImag = atof(argv[2]);
    if (argc > 3) tolerance = atof(argv[3]);
    if (argc > 4) MaxCodeLength = atoi(argv[4]);
    if (argc > 5) worker_id = atoi(argv[5]);
    if (argc > 6) total_workers = atoi(argv[6]);
    
    if (total_workers < 1) total_workers = 1;
    if (worker_id >= total_workers) worker_id = total_workers - 1;

    char ckpt_file[256];
    char log_file[256];
    snprintf(ckpt_file, sizeof(ckpt_file), "hpc_state_worker_%d.ckpt", worker_id);
    snprintf(log_file, sizeof(log_file), "candidates_worker_%d.log", worker_id);

    ComplexT targetC(tgReal, tgImag);
    double targetMagnitude = thrust::abs(targetC);

    printf("=== GPU EXHAUSTIVE Constant Recognition (HPC Complex Mode) ===\n");
    printf("Target Z:      %.17g + %.17gi\n", tgReal, tgImag);
    printf("Tolerance:     %.1e\n", tolerance);
    printf("Max K:         %d\n", MaxCodeLength);
    printf("Worker Info:   %d / %d\n", worker_id, total_workers);
    printf("Logs appended: %s\n", log_file);
    
    HPCState state = {1, 0, 0};
    if (load_hpc_state(ckpt_file, &state)) {
        printf(">>> RESUMING from Checkpoint! K=%d, FormId=%d, Offset=%llu\n", state.start_K, state.start_form_id, state.start_offset);
    }
    
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, 0);
    printf("GPU: %s\n\n", prop.name);
    
    ComplexT h_consts[N_CONST] = {
        ComplexT(3.14159265358979323846, 0),   // PI
        ComplexT(2.71828182845904523536, 0),   // E
        ComplexT(-1.0, 0),                     // NEG
        ComplexT(1.61803398874989484820, 0),   // PHI
        ComplexT(1.0, 0), ComplexT(2.0, 0), ComplexT(3.0, 0), ComplexT(4.0, 0), ComplexT(5.0, 0), 
        ComplexT(6.0, 0), ComplexT(7.0, 0), ComplexT(8.0, 0), ComplexT(9.0, 0),
        ComplexT(0.0, 1.0)                     // IMAGINARY UNIT 'I'
    };
    CUDA_CHECK(cudaMemcpyToSymbol(d_const_values, h_consts, N_CONST * sizeof(ComplexT)));

    Candidate* d_buffer;
    Candidate* h_buffer = (Candidate*)malloc(MAX_CANDIDATES * sizeof(Candidate));
    int* d_buffer_count;
    char* d_ternary; 
    int* d_radix;
    
    CUDA_CHECK(cudaMalloc(&d_buffer, MAX_CANDIDATES * sizeof(Candidate)));
    CUDA_CHECK(cudaMalloc(&d_buffer_count, sizeof(int)));
    CUDA_CHECK(cudaMalloc(&d_ternary, MAX_K + 1));
    CUDA_CHECK(cudaMalloc(&d_radix, MAX_K * sizeof(int)));
    
    int max_forms = 100000;
    FormDesc* all_forms = (FormDesc*)malloc(max_forms * sizeof(FormDesc));
    int* form_offsets = (int*)malloc((MaxCodeLength + 2) * sizeof(int));
    
    int total_forms = 0; form_offsets[0] = 0; form_offsets[1] = 0;
    for (int K = 1; K <= MaxCodeLength; K++) {
        int n = generate_valid_forms(K, all_forms + total_forms, max_forms - total_forms);
        total_forms += n;
        form_offsets[K + 1] = total_forms;
    }
    
    int threadsPerBlock = 256;
    unsigned long long chunk_size = 1ULL << 26;
    unsigned long long total_evaluated = 0;
    
    // Main loop logic observing Checkpoint state & Multi-node slicing
    for (int K = state.start_K; K <= MaxCodeLength; K++) {
        
        int form_start = form_offsets[K];
        
        if (state.start_K == K && state.start_form_id >= form_offsets[K]) {
            form_start = state.start_form_id;
        } else {
            // align form_start to worker subset
            while (form_start % total_workers != worker_id) form_start++;
        }
        
        for (int form_id = form_start; form_id < form_offsets[K + 1]; form_id += total_workers) {
            
            FormDesc* form = &all_forms[form_id];
            CUDA_CHECK(cudaMemcpy(d_ternary, form->ternary, K + 1, cudaMemcpyHostToDevice));
            CUDA_CHECK(cudaMemcpy(d_radix, form->radix, K * sizeof(int), cudaMemcpyHostToDevice));
            
            unsigned long long offset_start = (K == state.start_K && form_id == state.start_form_id) ? state.start_offset : 0;
            
            for (unsigned long long offset = offset_start; offset < form->total; offset += chunk_size) {
                
                // Clear Buffer Counter
                int h_count = 0;
                CUDA_CHECK(cudaMemcpy(d_buffer_count, &h_count, sizeof(int), cudaMemcpyHostToDevice));

                unsigned long long count = form->total - offset;
                if (count > chunk_size) count = chunk_size;
                
                int blocks = (count + threadsPerBlock - 1) / threadsPerBlock;
                search_form_kernel_exhaustive<<<blocks, threadsPerBlock>>>(
                    d_ternary, K, d_radix, count, offset, targetC, targetMagnitude, tolerance,
                    d_buffer, d_buffer_count, MAX_CANDIDATES, form_id
                );
                
                // Retrieve gathered candidates
                CUDA_CHECK(cudaMemcpy(&h_count, d_buffer_count, sizeof(int), cudaMemcpyDeviceToHost));
                
                if (h_count > 0) {
                    CUDA_CHECK(cudaMemcpy(h_buffer, d_buffer, h_count * sizeof(Candidate), cudaMemcpyDeviceToHost));
                    flush_candidates_to_disk(log_file, h_buffer, h_count, all_forms);
                }

                // SAVE Checkpoint atomically after chunk completes!
                state.start_K = K;
                state.start_form_id = form_id;
                state.start_offset = offset + count;
                save_hpc_state(ckpt_file, state);
            }
            total_evaluated += form->total;
        }
    }
    
    printf("\nExecution completed across partitioned space for worker %d!\n", worker_id);
    
    free(all_forms); free(form_offsets); free(h_buffer);
    cudaFree(d_buffer); cudaFree(d_buffer_count); cudaFree(d_ternary); cudaFree(d_radix);
    return 0;
}
