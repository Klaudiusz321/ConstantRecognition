/* Klaudiusz Sroka, 12.12.2025, klaudiusz.sroka@student.uj.edu.pl */

/*
To compile for WASM/WWW

emcc -Wall ConstantRecognition_hybrid_for_WASM.c ../C/constant.c ../C/mathematica.c ../C/math2.c \
-s WASM=1 -s EXPORTED_FUNCTIONS='["_search_RPN_hybrid", "_free"]' -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
-o rpn_function_hybrid.js

*/
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <complex.h>
#include <fenv.h>
#include <float.h>
#include "../C/constant.h"
#include "../C/itoa.c"
#include "../C/mathematica.h"
#include "../C/math2.h"
#include "../C/utils.h"

/* ============================================================================
   TYPE DEFINITIONS - HYBRID SETUP
   ============================================================================ */

// FP32 Context (Fast Pass)
#define NUM_TYPE_F float
#define ABS_F fabsf
#define CONSTANT_F constantf
#define EPSILON_F FLT_EPSILON
#define ONE_F  1.0f
#define ZERO_F 0.0f
#define IS_NAN_F(x) (isnan(x) || isinf(x))

// FP64 Context (Verification Pass - matches REAL_DBL from original)
#define NUM_TYPE_D double
#define ERR_TYPE_D double
#define MAX_NUMBER_D DBL_MAX
#define ABS_D fabs
#define CONSTANT_D constant 
#define EPSILON_D DBL_EPSILON
#define ONE_D  1.0
#define ZERO_D 0.0
#define IS_NAN_D isnan

#define EPS_MAX 16 
#define JSON_BUFFER_SIZE (1024*1024) 
#define min(a,b) ((a)<(b)?(a):(b))

static const char BUILD_TIMESTAMP[] = "BUILD_TIME:" __DATE__ " " __TIME__;

/* ============================================================================
   UTILITY FUNCTIONS (Copied from function2)
   ============================================================================ */

union DoubleInt64 {
    double d;
    uint64_t i;
};

int hamming_distance64(uint64_t x, uint64_t y) {
    return __builtin_popcountll(x ^ y);
}

double hamming_distance(double a, double b) {
    union DoubleInt64 ua, ub;
    ua.d = a;
    ub.d = b;
    int distance = hamming_distance64(ua.i, ub.i);
    return (double) distance;
}

ERR_TYPE_D why_not_ULP(const double ref, const double val) {
    if (isnan(ref) || isnan(val) || isinf(ref) || isinf(val)) {
        return (ERR_TYPE_D) UINT64_MAX;
    }
    int64_t i_ref, i_val;
    memcpy(&i_ref, &ref, sizeof(double));
    memcpy(&i_val, &val, sizeof(double));
    if (i_ref < 0) i_ref = INT64_MIN - i_ref;
    if (i_val < 0) i_val = INT64_MIN - i_val;
    uint64_t ulp_diff = (uint64_t)(llabs(i_val - i_ref));
    return ABS_D( (ERR_TYPE_D) ulp_diff );
}

ERR_TYPE_D rel_err_d(NUM_TYPE_D computedX, NUM_TYPE_D targetX) {
  if(targetX==ZERO_D)
    return ABS_D( computedX-targetX);
  else
    return ABS_D( computedX/targetX - ONE_D );
}

ERR_TYPE_D rankFunc(NUM_TYPE_D computedX, NUM_TYPE_D targetX) {
   return why_not_ULP(computedX, targetX);
}

int checkSyntax3(const char * ternary, const int length) {
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

const char* constants = "0123opqrstuvw";
const char* unary_funcs = "4589abcdefghijklmn";
const char* binary_ops = "67xyz";
int search_status = 1;

/* ============================================================================
   GLOBAL VARIABLES
   ============================================================================ */

char* RPN_full_Code;
char* JSON_output;
char* json_start;
int remaining; 
int written;
int result_count;
unsigned long long int j, k, k1=0, k2=0, kMAX, chunk_size, start, end;
char amino[STACKSIZE], amino_best[STACKSIZE], permutations[STACKSIZE];

// Dual variables for Hybrid approach
ERR_TYPE_D var, best = MAX_NUMBER_D, relative_error, Delta_z, compression_ratio;
NUM_TYPE_D computedX, targetX;    // FP64 targets
NUM_TYPE_F targetX_f;             // FP32 target
float threshold_f;                // Calculated threshold

// Counters for stats
unsigned long long total_fp32_evals = 0;
unsigned long long total_fp64_evals = 0;

int K, K_best=1, test;
const int n=3;

/* ============================================================================
   RECURSIVE HYBRID SEARCH
   ============================================================================ */

int generate_combinations_hybrid(char* ternary, char* result, int index, int length, int cpu_id, int* found) {

      if (*found) return 1;
        
      if (index == length) {
          total_fp32_evals++;

          // ---------------------------------------------------------
          // 1. FAST PASS (FP32)
          // ---------------------------------------------------------
                    // Calculate value in float (constantf)
          NUM_TYPE_F computedX_f = CONSTANT_F(result, K);
          
          if (IS_NAN_F(computedX_f)) return 0;

          // FP32 Filter Logic
                    // Check if: |computed - target| < threshold
          float diff = ABS_F(computedX_f - targetX_f);
          
                    // If the difference is too large, reject IMMEDIATELY.
                    // Do not perform expensive double calculations.
          if (diff >= threshold_f) {
              return 0; 
          }

          // ---------------------------------------------------------
          // 2. VERIFICATION PASS (FP64)
          // Wykonywane tylko dla obiecujących kandydatów
          // ---------------------------------------------------------
          total_fp64_evals++;
          
          computedX = CONSTANT_D(result, K);
          if (IS_NAN_D(computedX)) return 0;

          k2++;
          var = rankFunc(computedX, targetX);
      
          if(var < best) {
            best = var;
            K_best = K;
            strncpy(amino_best, result, K_best);
            print_code_mathematica(amino_best, K_best, RPN_full_Code);
            
            if (result_count > 0) {
                written = snprintf(json_start, remaining, ",");
                json_start += written;
                remaining -= written;
            }
             
            relative_error = rel_err_d(computedX, targetX);

            // Compression Ratio Logic (from function2)
            if (targetX == 0.0) {
                compression_ratio = 0.0;
            } else {
                if (relative_error == 0.0) {
                    double digitsInTarget = 16.0;
                    double informationInRPN = K * log10(INSTR_NUM);
                    if (informationInRPN <= 0.0) compression_ratio = 0.0;
                    else compression_ratio = digitsInTarget / informationInRPN;
                } else {
                    double digitsInTarget = -log10(relative_error);
                    double informationInRPN = K * log10(INSTR_NUM);
                    if (relative_error >= 1.0 || informationInRPN <= 0.0) compression_ratio = 0.0;
                    else compression_ratio = digitsInTarget / informationInRPN;
                }
            }

            written = snprintf(json_start, remaining, 
                "{\"RPN\":\"%s\", \"REL_ERR\":%.17e, \"K\":%d, \"result\":\"INTERMEDIATE\", \"status\":\"RUNNING\", \"cpuId\":%d, \"HAMMING_DISTANCE\":%lf, \"fp32_evals\":%llu, \"fp64_evals\":%llu}",
                RPN_full_Code, relative_error, K_best, cpu_id, 
                hamming_distance(computedX, targetX),
                total_fp32_evals, total_fp64_evals); // Added stats to JSON
            
            json_start += written;
            remaining -= written;
            result_count++;
          }

          if ((relative_error <= EPS_MAX * EPSILON_D) || 
              (ABS_D(computedX - targetX) <= 2.0 * Delta_z && compression_ratio >= 1.05)) 
          {
            print_code_mathematica(amino_best, K_best, RPN_full_Code);

            written = snprintf(json_start, remaining, 
                "], \"result\":\"SUCCESS\", \"RPN\":\"%s\", \"REL_ERR\":%.17e, \"INPUT_ABS_ERR\":%lf, \"COMPRESSION_RATIO\":%lf, \"K\":%d, \"status\":\"FINISHED\", \"HAMMING_DISTANCE\":%lf}",
                RPN_full_Code, relative_error, Delta_z, compression_ratio, K_best, hamming_distance(computedX, targetX));
            
            *found = 1;            
            return 1;
          }
          return 0;
      }
  
      const char* options;
      int options_length;
  
      switch(ternary[index]) {
          case '0': options = constants; options_length = strlen(constants); break;
          case '1': options = unary_funcs; options_length = strlen(unary_funcs); break;
          case '2': options = binary_ops; options_length = strlen(binary_ops); break;
      }
  
      for (int i = 0; i < options_length; i++) {
          result[index] = options[i];
          if(generate_combinations_hybrid(ternary, result, index + 1, length, cpu_id, found)) return 1;
      }
      return 0;
  }

/* ============================================================================
   MAIN ENTRY POINT
   ============================================================================ */

char* search_RPN_hybrid(double z, double dz, int MinCodeLength, int MaxCodeLength, int cpu_id, int ncpus) {

  // Allocate memory
  RPN_full_Code = (char*)malloc(32 * 16 * sizeof(char));
  JSON_output = (char*)malloc(JSON_BUFFER_SIZE * sizeof(char));
  if ((RPN_full_Code == NULL) || (JSON_output == NULL)) return "{\"error\": \"Memory allocation failed\"}";

  // Initialize JSON
  json_start = JSON_output;
  remaining = JSON_BUFFER_SIZE;
  written = snprintf(json_start, remaining, 
      "{\"cpuId\":%d, \"buildTime\":\"%s\", \"mode\":\"HYBRID\", \"results\": [", cpu_id, BUILD_TIMESTAMP);
  json_start += written;
  remaining -= written;

  result_count = 0;
  total_fp32_evals = 0;
  total_fp64_evals = 0;

  // Setup Targets for FP64 and FP32
  targetX = (NUM_TYPE_D)z;
  targetX_f = (NUM_TYPE_F)z;
  Delta_z = (NUM_TYPE_D)dz;
  
  // Calculate FP32 Threshold: 24 * FLT_EPSILON
  // Scaled by target magnitude to handle large numbers (like 77777) correctly
  // Logic: |val - target| < 24 * EPS * |target|
  threshold_f = 24.0f * FLT_EPSILON;
  if (ABS_F(targetX_f) > FLT_MIN) {
      threshold_f *= ABS_F(targetX_f);
  }
  // Ensure minimal threshold for near-zero targets
  if (threshold_f < 24.0f * FLT_EPSILON) threshold_f = 24.0f * FLT_EPSILON;


  best = MAX_NUMBER_D;  
  strcpy(amino_best, "0");
  int found = 0;

  j = cpu_id;
  for(K = MinCodeLength; K <= MaxCodeLength; K++) {

    kMAX = ipow(3, K);
    chunk_size = (kMAX / ncpus) + ((kMAX % ncpus) > cpu_id ? 1 : 0);
    start = cpu_id * (kMAX / ncpus) + min(cpu_id, kMAX % ncpus);
    end = start + chunk_size;

    for(k = start; k < end; k++) {		
      j = j + 1;
      itoa(k, amino, 3, K);
          
      test = checkSyntax3(amino, K);
      if (!test) continue;
      k1++;

      generate_combinations_hybrid(amino, permutations, 0, K, cpu_id, &found);
      if (found) return JSON_output;  
    }
    
    // Check for "Pointless Search" / Early Exit (Exact logic from function2)
    if((k1 <= 12ULL) && (j > 250LL) && (K > 4)) {
      print_code_mathematica(amino_best, K_best, RPN_full_Code);
      computedX = CONSTANT_D(amino_best, K_best);
      best = rankFunc(computedX, targetX);
      
      written = snprintf(json_start, remaining, 
        "], \"result\":\"ABORTED\", \"RPN\":\"%s\", \"REL_ERR\":%.17e, \"K\":%d, \"status\":\"FINISHED\", \"HAMMING_DISTANCE\":%lf}",
        RPN_full_Code, rel_err_d(computedX, targetX), K_best, hamming_distance(computedX, targetX));
      return JSON_output;
    }

    if (result_count > 0) {
        written = snprintf(json_start, remaining, ",");
        json_start += written;
        remaining -= written;
    }
    
    print_code_mathematica(amino_best, K_best, RPN_full_Code);
    computedX = CONSTANT_D(amino_best, K_best);
    relative_error = rel_err_d(computedX, targetX);

    written = snprintf(json_start, remaining, 
       "{\"RPN\":\"%s\", \"REL_ERR\":%.17e, \"K\":%d, \"result\":\"K_BEST\", \"status\":\"RUNNING\", \"cpuId\":%d, \"HAMMING_DISTANCE\":%lf, \"fp32_evals\":%llu, \"fp64_evals\":%llu}",
       RPN_full_Code, relative_error, K, cpu_id, 
       hamming_distance(computedX, targetX),
       total_fp32_evals, total_fp64_evals);
    json_start += written;
    remaining -= written;
    result_count++;
  }

  // Fallback / Not Found
  print_code_mathematica(amino_best, K_best, RPN_full_Code);
  computedX = CONSTANT_D(amino_best, K_best);
  
  written = snprintf(json_start, remaining, 
    "], \"result\":\"FAILURE\", \"RPN\":\"%s\", \"REL_ERR\":%.17e, \"K\":%d, \"status\":\"FINISHED\", \"HAMMING_DISTANCE\":%lf}",
    RPN_full_Code, rel_err_d(computedX, targetX), K_best, hamming_distance(computedX, targetX));

  return JSON_output;
}