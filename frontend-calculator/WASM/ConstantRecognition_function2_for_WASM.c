/* Andrzej Odrzywolek, 02.09.2024, andrzej.odrzywolek@uj.edu.pl */

/*
To compile for WASM/WWW

emcc -Wall ConstantRecognition_function2_for_WASM.c ../C/constant.c ../C/mathematica.c ../C/math2.c \
-s WASM=1 -s EXPORTED_FUNCTIONS='["_search_RPN", "_free"]' -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
-o rpn_function.js

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

// Define your type options
#define REAL_FLT  1
#define REAL_DBL  2
#define REAL_LDBL 3
#define CPLX_FLT  4
#define CPLX_DBL  5
#define CPLX_LDBL 6

#define SEARCH_TYPE REAL_DBL

#if   SEARCH_TYPE == REAL_FLT
  #define NUM_TYPE float
  #define ERR_TYPE float
  #define MAX_NUMBER FLT_MAX
  #define ABS fabsf
  #define CONSTANT constantf
  #define EPSILON FLT_EPSILON
  #define ONE  1.0f
  #define ZERO 0.0f
  #define IS_NAN isnanf
#elif SEARCH_TYPE == REAL_DBL
  #define NUM_TYPE double
  #define ERR_TYPE double
  #define MAX_NUMBER DBL_MAX
  #define ABS fabs
  #define CONSTANT constant 
  #define EPSILON DBL_EPSILON
  #define ONE  1.0
  #define ZERO 0.0
  #define IS_NAN isnan
#elif SEARCH_TYPE == REAL_LDBL
  #define NUM_TYPE long double
  #define ERR_TYPE long double
  #define MAX_NUMBER LDBL_MAX
  #define ABS fabsl
  #define CONSTANT constantl
  #define EPSILON LDBL_EPSILON
  #define ONE  1.0l
  #define ZERO 0.0l
  #define IS_NAN isnanl
#elif SEARCH_TYPE == CPLX_FLT
  #define NUM_TYPE complex float
  #define ERR_TYPE float
  #define MAX_NUMBER FLT_MAX
  #define ABS cabsf
  #define CONSTANT cconstantf
  #define EPSILON FLT_EPSILON
  #define ONE ( 1.0f + 0.0f * I)
  #define ZERO (0.0f + 0.0f * I)
  #define IS_NAN isnanf
#elif SEARCH_TYPE == CPLX_DBL
  #define NUM_TYPE complex double
  #define ERR_TYPE double
  #define MAX_NUMBER DBL_MAX
  #define ABS cabs
  #define CONSTANT cconstantf
  #define EPSILON DBL_EPSILON
  #define ONE  (1.0 + 0.0 * I)
  #define ZERO (0.0 + 0.0 * I)
  #define IS_NAN isnan
#elif SEARCH_TYPE == CPLX_LDBL
  #define NUM_TYPE complex long double
  #define ERR_TYPE long double
  #define MAX_NUMBER LDBL_MAX
  #define ABS cabsl
  #define CONSTANT cconstantl
  #define EPSILON LDBL_EPSILON
  #define ONE  (1.0L + 0.0L * I)
  #define ZERO (0.0L + 0.0L * I)
  #define IS_NAN isnanl
#endif


#define EPS_MAX 16 //Maximum error in DBL_EPSILON considered to be equality, use 0 or 1 to be "paranoid"
#define JSON_BUFFER_SIZE (1024*1024)  // 1MB
#define min(a,b) ((a)<(b)?(a):(b))

static const char BUILD_TIMESTAMP[] = "BUILD_TIME:" __DATE__ " " __TIME__;


union DoubleInt64 {
    double d;
    uint64_t i;
};

// Hamming distance for 64-bit integers
int hamming_distance64(uint64_t x, uint64_t y)
{
    return __builtin_popcountll(x ^ y);
}

// Similarity function based on Hamming distance
double hamming_distance(double a, double b)
{
    union DoubleInt64 ua, ub;
    ua.d = a;
    ub.d = b;
    
    int distance = hamming_distance64(ua.i, ub.i);
    
    // Convert distance to similarity (64 is the total number of bits)
    return (double) distance;
}

//Proposed by o1

ERR_TYPE why_not_ULP(const double ref, const double val)
{
    // Handle NaN and infinities if necessary
    if (isnan(ref) || isnan(val) || isinf(ref) || isinf(val)) {
        // Define behavior for NaN
        return (ERR_TYPE) UINT64_MAX;
    }

    // Map doubles to int64_t while preserving ordering
    int64_t i_ref, i_val;
    memcpy(&i_ref, &ref, sizeof(double));
    memcpy(&i_val, &val, sizeof(double));

    if (i_ref < 0)
        i_ref = INT64_MIN - i_ref;
    if (i_val < 0)
        i_val = INT64_MIN - i_val;

    uint64_t ulp_diff = (uint64_t)(llabs(i_val - i_ref));
    return ABS( (ERR_TYPE) ulp_diff );
}

ERR_TYPE rel_err(NUM_TYPE computedX, NUM_TYPE targetX)
{
  if(targetX==ZERO)
    return ABS( computedX-targetX);
  else
    return ABS( computedX/targetX - ONE );

}

//typedef double (*RankingFunction)(NUM_TYPE computedX, NUM_TYPE targetX);

ERR_TYPE rankFunc(NUM_TYPE computedX, NUM_TYPE targetX)
{
   
   //return rel_err(computedX, targetX);
   //return hamming_distance(computedX, targetX);
   return why_not_ULP(computedX, targetX);
}

int checkSyntax3(const char * ternary, const int length) {
    int stack = 0;
    for (int i = 0; i < length; i++) {
        switch(ternary[i]) {
            case '0': // constant
                stack++;
                break;
            case '1': // unary function
                if (stack < 1) return 0;
                break;
            case '2': // binary operation
                if (stack < 2) return 0;
                stack--;
                break;
        }
    }
    return (stack == 1);
}


//void generate_combinations(char*, char*, int, int, int);
const char* constants = "0123opqrstuvw";  // 0-9, pi, e, -1, GoldenRatio, etc...
const char* unary_funcs = "4589abcdefghijklmn";  // log, exp, inv, minus, sqrt, sqr, trig functions, etc...
const char* binary_ops = "67xyz";  // plus, times, subtract, divide, power, etc..
int search_status = 1; //Running

//Global variables

char* RPN_full_Code;
char* JSON_output;
char* json_start;
int remaining; 
int written;
int result_count;
unsigned long long int j, k, k1=0, k2=0,kMAX,chunk_size,start,end;
char amino[STACKSIZE], amino_best[STACKSIZE], permutations[STACKSIZE];

ERR_TYPE var, best = MAX_NUMBER, relative_error, Delta_z, compression_ratio;
NUM_TYPE computedX, targetX;
 
int K, K_best=1, test;
const int n=3;


int generate_combinations(char* ternary, char* result, int index, int length, int cpu_id, int* found) {


      if (*found) {
        return 1;  //stop recursion
      }
        
      if (index == length) {
          // Process the complete combination
      computedX = CONSTANT(result, K);

	  if (IS_NAN(computedX)) return 0;  // Skip NaN
      k2++;
      //var = ABS( computedX/targetX - ONE );
      var = rankFunc(computedX, targetX);

      //printf("var=%lf\n",var);
      
      if(var<best) 
       {
        best = var;
        K_best=K;
        strncpy(amino_best,result,K_best);
        print_code_mathematica(amino_best,K_best,RPN_full_Code);
        

        if (result_count > 0) 
         {
          written = snprintf(json_start, remaining, ",");
          json_start += written;
          remaining -= written;
         }
         
        relative_error = rel_err(computedX, targetX);


        if (targetX == 0.0) {
            compression_ratio = 0.0; // Handle cases where the target is 0
        } 
        else {

          if (relative_error == 0.0) {
              // Perfect match (special case)
              double digitsInTarget = floor(log10(fabs(targetX))) + 1.0;
              double informationInRPN = K * log10(INSTR_NUM);
		  
              if (informationInRPN <= 0.0) {
                  compression_ratio = 0.0; // Or handle it as an error
              } else {
                  compression_ratio = digitsInTarget / informationInRPN;
              }
          } else {
              // General case (approximation)
              double digitsInTarget = -log10(relative_error);
              double informationInRPN = K * log10(INSTR_NUM);
		  
              if (relative_error >= 1.0 || informationInRPN <= 0.0) {
                  compression_ratio = 0.0; // Or handle it as a very poor approximation
              } else {
                  compression_ratio = digitsInTarget / informationInRPN;
              }
          }
        }


        written = snprintf(json_start, remaining, 
            "{\"RPN\":\"%s\", \"REL_ERR\":%.17e, \"K\":%d, \"result\":\"INTERMEDIATE\", \"status\":\"RUNNING\", \"cpuId\":%d, \"HAMMING_DISTANCE\":%lf}",
            RPN_full_Code, relative_error, K_best,cpu_id, hamming_distance(computedX, targetX));
          json_start += written;
          remaining -= written;

        result_count++;

	   }

       
    
	   if(   (relative_error<=EPS_MAX*EPSILON)    
           ||  ( ABS(computedX-targetX)<=2.0*Delta_z && compression_ratio>=1.05 ) 
                                                          ) //jezeli znalazl, wychodzi z petli i funkcji !
	   {
	    
        print_code_mathematica(amino_best,K_best,RPN_full_Code);

          // Immediate success, finalize JSON and return
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
          case '0':
              options = constants;
              options_length = strlen(constants);
              break;
          case '1':
              options = unary_funcs;
              options_length = strlen(unary_funcs);
              break;
          case '2':
              options = binary_ops;
              options_length = strlen(binary_ops);
              break;
      }
  
      for (int i = 0; i < options_length; i++) {
          result[index] = options[i];
          if(generate_combinations(ternary, result, index + 1, length, cpu_id, found)) return 1;
      }
      return 0;
  }




char* search_RPN(double z, double dz, int MinCodeLength, int MaxCodeLength, int cpu_id, int ncpus) {

  // Allocate memory for the output string
  RPN_full_Code = (char*)malloc(32*16 * sizeof(char));
  JSON_output =  (char*)malloc(JSON_BUFFER_SIZE * sizeof(char));
  if( (RPN_full_Code == NULL) || (JSON_output == NULL) ) return "Error allocating memory for JSON";

  // Initialize JSON output
  json_start = JSON_output;
  remaining = JSON_BUFFER_SIZE;
  written = snprintf(json_start, remaining, 
      "{\"cpuId\":%d, \"buildTime\":\"%s\", \"results\": [", cpu_id, BUILD_TIMESTAMP);
  json_start += written;
  remaining -= written;

  result_count = 0;
  

  targetX = ( NUM_TYPE ) z;
  Delta_z = (NUM_TYPE) dz;
  best  = MAX_NUMBER;  
  strcpy(amino_best, "0");  // Initialize with default string equivalent to a first constant
  int found = 0;

  
  j=cpu_id;
  for(K=MinCodeLength;K<=MaxCodeLength;K++)
  {

    kMAX = ipow(3, K);
    chunk_size = (kMAX / ncpus) + ((kMAX % ncpus) > cpu_id ? 1 : 0);
    start = cpu_id * (kMAX / ncpus) + min(cpu_id, kMAX % ncpus);
    end = start + chunk_size;

    //printf("K=%d,\tkMAX=%llu,\tchunk_size=%llu, start=%llu, end=%llu\n",K,kMAX,chunk_size, start,end);

    for(k=start;k<end;k++)
    //for(k=cpu_id;k<kMAX;k=k+ncpus)
    {		
      //j=j+ncpus;
      j=j+1;
      /* Convert number 'k' into string 'amino' in base-3 number of length 'K' including leading zeros */
      itoa(k, amino, 3, K);
          
      test = checkSyntax3 (amino, K); //check if base-3 RPN code is valid 
     
      //printf("%llu\t%llu\t%llu\n", k, j, k1);
      if (!test) continue;
	  k1++;

      //printf("%d\t%llu\t%s\n\n\n\n\n",K,k,amino);
      generate_combinations(amino, permutations, 0, K, cpu_id, &found);
      //printf("Search status: %d\n\n\n\n\n", search_status);
      if( found) return JSON_output;  
     

    }
    if((k1<=12ULL) && (j>250LL) && (K>4) ) //Early exit if basically NOTHING was found so far; Further search seems pointless. 
    {
      
      print_code_mathematica(amino_best,K_best,RPN_full_Code);
      computedX = CONSTANT(amino_best, K_best);
	  best = rankFunc(computedX, targetX);
      //strcat(RPN_full_Code, ", FAILURE");
      //printf("\nk1=%llu\tj=%llu\n",k1,j);
      //sprintf(REL_ERR_string, "%.17e",best);
      //sprintf(JSON_output, "{\"result\":\"ABORTED\", \"RPN\":\"%s\", \"REL_ERR\":\"%s\", \"status\":\"%s\"}",RPN_full_Code,REL_ERR_string,"FINISHED");
       written = snprintf(json_start, remaining, 
        "], \"result\":\"ABORTED\", \"RPN\":\"%s\", \"REL_ERR\":%.17e, \"K\":%d, \"status\":\"FINISHED\", \"HAMMING_DISTANCE\":%lf}",
        RPN_full_Code, rel_err(computedX, targetX), K_best, hamming_distance(computedX, targetX));
      return JSON_output;
    }

   if (result_count > 0) 
    {
     written = snprintf(json_start, remaining, ",");
     json_start += written;
     remaining -= written;
    }
    
  
  
  print_code_mathematica(amino_best,K_best,RPN_full_Code);
  computedX = CONSTANT(amino_best, K_best);
  relative_error = rel_err(computedX, targetX);

   written = snprintf(json_start, remaining, 
       "{\"RPN\":\"%s\", \"REL_ERR\":%.17e, \"K\":%d, \"result\":\"K_BEST\", \"status\":\"RUNNING\", \"cpuId\":%d, \"HAMMING_DISTANCE\":%lf}",
       RPN_full_Code, relative_error, K, cpu_id, hamming_distance(computedX, targetX));
     json_start += written;
     remaining -= written;
   result_count++;


  }

  /* WARNING: it is possible for search to find NOTHING! 
     Returning k=0, K=1 in this cases as fallback.
  */
  
  
  print_code_mathematica(amino_best,K_best,RPN_full_Code);
  computedX = CONSTANT(amino_best, K_best);
  
  //printf("\nk1=%llu\tj=%llu\n",k1,j);


  // Finalize JSON output for failure case
  written = snprintf(json_start, remaining, 
    "], \"result\":\"FAILURE\", \"RPN\":\"%s\", \"REL_ERR\":%.17e, \"K\":%d, \"status\":\"FINISHED\", \"HAMMING_DISTANCE\":%lf}",
    RPN_full_Code, rel_err(computedX, targetX), K_best, hamming_distance(computedX, targetX));



  return JSON_output;

}