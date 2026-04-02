/* Andrzej Odrzywolek, 03.01.2024, andrzej.odrzywolek@uj.edu.pl */

/*
To compile for WASM/WWW

emcc -Wall ConstantRecognition_function_for_WASM.c ../C/constant.c ../C/itoa.c ../C/mathematica.c ../C/math2.c \
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
  #define ONE 1.0f
  #define IS_NAN isnanf
#elif SEARCH_TYPE == REAL_DBL
  #define NUM_TYPE double
  #define ERR_TYPE double
  #define MAX_NUMBER DBL_MAX
  #define ABS fabs
  #define CONSTANT constant 
  #define EPSILON DBL_EPSILON
  #define ONE 1.0
  #define IS_NAN isnan
#elif SEARCH_TYPE == REAL_LDBL
  #define NUM_TYPE long double
  #define ERR_TYPE long double
  #define MAX_NUMBER LDBL_MAX
  #define ABS fabsl
  #define CONSTANT constantl
  #define EPSILON LDBL_EPSILON
  #define ONE 1.0l
  #define IS_NAN isnanl
#elif SEARCH_TYPE == CPLX_FLT
  #define NUM_TYPE complex float
  #define ERR_TYPE float
  #define MAX_NUMBER FLT_MAX
  #define ABS cabsf
  #define CONSTANT cconstantf
  #define EPSILON FLT_EPSILON
  #define ONE 1.0f
  #define IS_NAN isnanf
#elif SEARCH_TYPE == CPLX_DBL
  #define NUM_TYPE complex double
  #define ERR_TYPE double
  #define MAX_NUMBER DBL_MAX
  #define ABS cabs
  #define CONSTANT cconstantf
  #define EPSILON DBL_EPSILON
  #define ONE 1.0
  #define IS_NAN isnan
#elif SEARCH_TYPE == CPLX_LDBL
  #define NUM_TYPE complex long double
  #define ERR_TYPE long double
  #define MAX_NUMBER LDBL_MAX
  #define ABS cabsl
  #define CONSTANT cconstantl
  #define EPSILON LDBL_EPSILON
  #define ONE 1.0l
  #define IS_NAN isnanl
#endif


#define EPS_MAX 16 //Maximum error in DBL_EPSILON considered to be equality, use 0 or 1 to be "paranoid"
#define JSON_BUFFER_SIZE (1024*1024)  // 1MB
#define min(a,b) ((a)<(b)?(a):(b))



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

ERR_TYPE rel_err(NUM_TYPE computedX, NUM_TYPE targetX)
{
  return ABS( computedX/targetX - ONE );
}

//typedef double (*RankingFunction)(NUM_TYPE computedX, NUM_TYPE targetX);

ERR_TYPE rankFunc(NUM_TYPE computedX, NUM_TYPE targetX)
{
   
   return rel_err(computedX, targetX);
   //return hamming_distance(computedX, targetX);
}

char* search_RPN(double z, double Delta_z, int MinCodeLength, int MaxCodeLength, int cpu_id, int ncpus) {


  // Allocate memory for the output string
  char* RPN_full_Code = (char*)malloc(32*16 * sizeof(char));
  char* JSON_output =  (char*)malloc(JSON_BUFFER_SIZE * sizeof(char));
  

  if( (RPN_full_Code == NULL) || (JSON_output == NULL) ) return "Error allocating memory for JSON";

    // Initialize JSON output
    char* json_start = JSON_output;
    int remaining = JSON_BUFFER_SIZE;
    int written = snprintf(json_start, remaining, 
        "{\"cpuId\":%d, \"results\": [", cpu_id);
    json_start += written;
    remaining -= written;



    int result_count = 0;
    //const int MAX_RESULTS = 128;  // Adjust as needed

  
  unsigned long long int j, k, k_best=0, k1=0, k2=0,kMAX,chunk_size,start,end;
  
  char amino[STACKSIZE];

  
  ERR_TYPE var, best, relative_error, compression_ratio;
  NUM_TYPE computedX, targetX;
  

   
  int K, K_best=1, test;
  const int n=INSTR_NUM;
  //int omp_cancel_flag=0, cpu_id=0, ncpus=1;
  
  //FILE  *flagfile, *search_log_file;  
  //char str[137], output_filename[137], timestamp[26], RPN_full_Code[1024];

  targetX = ( NUM_TYPE ) z;
  


  best  = MAX_NUMBER;
  
  j=cpu_id;
  for(K=MinCodeLength;K<=MaxCodeLength;K++)
  {
    //kMAX=ipow(INSTR_NUM,K);
    //printf("DEBUG: %d\n\n",kMAX);
    //chunk_size = (kMAX/ncpus)+0ULL;
    //start=cpu_id*chunk_size;
    //end = (cpu_id == ncpus-1) ? kMAX : start+chunk_size-1;


    //kMAX = ipow(INSTR_NUM, K);
    //chunk_size = (kMAX / ncpus) + (kMAX % ncpus ? 1 : 0); // Ensure all work is covered
    //start = cpu_id * chunk_size;
    //end = (cpu_id == ncpus - 1) ? kMAX : (start + chunk_size); // Last CPU takes any remainder

    kMAX = ipow(INSTR_NUM, K);
    chunk_size = (kMAX / ncpus) + ((kMAX % ncpus) > cpu_id ? 1 : 0);
    start = cpu_id * (kMAX / ncpus) + min(cpu_id, kMAX % ncpus);
    end = start + chunk_size;

    //printf("K=%d,\tkMAX=%llu,\tchunk_size=%llu, start=%llu, end=%llu\n",K,kMAX,chunk_size, start,end);

    for(k=start;k<end;k++)
    //for(k=cpu_id;k<kMAX;k=k+ncpus)
    {		
      //printf("DEBUG: %d\n",k);
      if(k==start) itoa(start, amino, n, K); else itoa_update(amino, n, K);
      //j=j+ncpus;
      j=j+1;
      /* Convert number 'k' into string 'amino' in base-n number of length 'K' including leading zeros */
      //itoa(k, amino, n, K);
          
      test = checkSyntax (amino, K); //check if RPN code is valid 
      
      if (!test) continue;
	  k1++;
    
      computedX = CONSTANT(amino, K);
	  if (IS_NAN(computedX)) continue;  // Skip NaN
      k2++;
      //var = ABS( computedX/targetX - ONE );	  
      var = rankFunc(computedX, targetX);	  
      
      if(var<best) 
       {
        best = var;
        K_best=K;
        k_best=k;
        print_code_mathematica(amino,K_best,RPN_full_Code);
        

        if (result_count > 0) 
         {
          written = snprintf(json_start, remaining, ",");
          json_start += written;
          remaining -= written;
         }
         
        relative_error = rel_err(computedX, targetX);

        if(Delta_z==0.0)
         {

          double base10DigitCount = floor(log10(fabs(targetX))) + 1.0;
        // Adjust for the information content difference between base 10 and base 36
          double adjustedBase10Count = base10DigitCount * log10(10) / log10(36);
          compression_ratio = adjustedBase10Count / K;
         }
        else
         {
          compression_ratio = - log10( fmax(best,Delta_z)/z)/(K)/log10(INSTR_NUM);
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
	    itoa(k_best, amino, n, K_best);
        print_code_mathematica(amino,K_best,RPN_full_Code);

          // Immediate success, finalize JSON and return
          written = snprintf(json_start, remaining, 
            "], \"result\":\"SUCCESS\", \"RPN\":\"%s\", \"REL_ERR\":%.17e, \"INPUT_ABS_ERR\":%lf, \"COMPRESSION_RATIO\":%lf, \"K\":%d, \"status\":\"FINISHED\", \"HAMMING_DISTANCE\":%lf}",
            RPN_full_Code, relative_error, Delta_z, compression_ratio, K_best, hamming_distance(computedX, targetX));
                    
        return JSON_output;
       }
       

    }
    if((k1<=12ULL) && (j>1000000ULL) && (K>4) ) //Early exit if basically NOTHING was found so far; Further search seems pointless. Used values are for 36-button CALC4
    {
      itoa(k_best, amino, n, K_best);
      print_code_mathematica(amino,K_best,RPN_full_Code);
      computedX = CONSTANT(amino, K_best);
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
    
  
  itoa(k_best, amino, n, K_best);
  print_code_mathematica(amino,K_best,RPN_full_Code);
  computedX = CONSTANT(amino, K_best);
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
  
  itoa(k_best, amino, n, K_best);
  print_code_mathematica(amino,K_best,RPN_full_Code);
  computedX = CONSTANT(amino, K_best);
  
  //printf("\nk1=%llu\tj=%llu\n",k1,j);


  // Finalize JSON output for failure case
  written = snprintf(json_start, remaining, 
    "], \"result\":\"FAILURE\", \"RPN\":\"%s\", \"REL_ERR\":%.17e, \"K\":%d, \"status\":\"FINISHED\", \"HAMMING_DISTANCE\":%lf}",
    RPN_full_Code, rel_err(computedX, targetX), K_best, hamming_distance(computedX, targetX));

  return JSON_output;

}
