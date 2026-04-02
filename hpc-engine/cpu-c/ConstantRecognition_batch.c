/* Andrzej Odrzywolek, 15.12.2023, andrzej.odrzywolek@uj.edu.pl */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <complex.h>
#include <fenv.h>
//#include <omp.h>
#include <float.h>
#include "constant.h"
#include "itoa.c"
#include "mathematica.h"
#include "math2.h"
#include "utils.h"
#include <time.h>


/*
#ifdef USE_COMPLEX
  #define NUM_TYPE double complex
#else
  #define NUM_TYPE double
#endif
*/
// Define your type options
#define REAL_FLT  1
#define REAL_DBL  2
#define REAL_LDBL 3
#define CPLX_FLT  4
#define CPLX_DBL  5
#define CPLX_LDBL 6

//#define SEARCH_TYPE REAL_FLT

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
  #define USE_COMPLEX
  #define NUM_TYPE complex float
  #define ERR_TYPE float
  #define MAX_NUMBER FLT_MAX
  #define ABS cabsf
  #define CONSTANT cconstantf
  #define EPSILON FLT_EPSILON
  #define ONE  (1.0f + 0.0f * I)
  #define ZERO (0.0f + 0.0f * I)
  #define IS_NAN isnanf
#elif SEARCH_TYPE == CPLX_DBL
  #define USE_COMPLEX
  #define NUM_TYPE complex double
  #define ERR_TYPE double
  #define MAX_NUMBER DBL_MAX
  #define ABS cabs
  #define CONSTANT cconstant
  #define EPSILON DBL_EPSILON
  #define ONE  (1.0 + 0.0 * I)
  #define ZERO (0.0 + 0.0 * I)
  #define IS_NAN isnan
#elif SEARCH_TYPE == CPLX_LDBL
  #define USE_COMPLEX
  #define NUM_TYPE complex long double
  #define ERR_TYPE long double
  #define MAX_NUMBER LDBL_MAX
  #define ABS cabsl
  #define CONSTANT cconstantl
  #define EPSILON LDBL_EPSILON
  #define ONE (1.0L + 0.0L * I)
  #define ZERO (0.0L + 0.0L * I)
  #define IS_NAN isnanl
#endif


#define EPS_MAX 4 //Maximum error considered to be equality, use 0 or 1 to be "paranoid"

ERR_TYPE rel_err(NUM_TYPE computedX, NUM_TYPE targetX)
{
  
  if(targetX==ZERO)
    return ABS( computedX-targetX);
  else
    return ABS( computedX/targetX - ONE );
}

// Helper function to evaluate polynomial combinations up to 4th power
ERR_TYPE evaluate_relationships(NUM_TYPE computedX, NUM_TYPE targetX) {
    const int MAX_COEFF = 8; // Maximum coefficient to try
    ERR_TYPE best_err = MAX_NUMBER;
    
    // Try combinations ax + bx² + cx³ + dx^4 where a,b,c,d are non-negative integers
    for(int a = 0; a <= MAX_COEFF; a++) {
        for(int b = 0; b <= MAX_COEFF; b++) {
            for(int c = 0; c <= MAX_COEFF; c++) {
                for(int d = 0; d <= MAX_COEFF; d++) {
                    // Skip trivial case
                    if(a == 0 && b == 0 && c == 0 && d == 0) continue;
                    
                    // Calculate x², x³, and x⁴
                    NUM_TYPE x2 = computedX * computedX;
                    NUM_TYPE x3 = x2 * computedX;
                    NUM_TYPE x4 = x3 * computedX;
                    
                    // Calculate ax + bx² + cx³ + dx⁴
                    NUM_TYPE candidate = a*computedX + b*x2 + c*x3 + d*x4;
                    ERR_TYPE err = ABS(candidate/targetX - ONE);
                    
                    if(err < best_err) {
                        best_err = err;
#ifdef DEBUG
                        printf("Better approximation found: %d*x + %d*x² + %d*x³ + %d*x⁴ = %f (error: %e)\n", 
                               a, b, c, d, (double)candidate, (double)err);
#endif
                    }
                }
            }
        }
    }
    
    return best_err;
}

//typedef double (*RankingFunction)(NUM_TYPE computedX, NUM_TYPE targetX);

ERR_TYPE rankFunc(NUM_TYPE computedX, NUM_TYPE targetX)
{
   
   return rel_err(computedX, targetX);
   //return evaluate_relationships(computedX, targetX);
   //return hamming_distance(computedX, targetX);
}

int main(int argc, char** argv)
{
  unsigned long long int j=0, k=0, k1=0, k2=0, kMAX;
  
  char amino[STACKSIZE];
  #ifdef USE_COMPLEX
  // DO NOTHING...
  #else
  double z;
  #endif
  ERR_TYPE var, best;
  NUM_TYPE computedX, targetX;
  

   
  int K, test, MaxCodeLength=6;
  uint64_t ULP;
  const int n=INSTR_NUM;
  int omp_cancel_flag=0, cpu_id=1, ncpus=1;
  
  FILE  *flagfile, *search_log_file;  
  char str[137], output_filename[137], timestamp[26], RPN_full_Code[1024];


  if(!(argv[1]==NULL))
  {
    sscanf(argv[1],"%s", str);
    sscanf(argv[2],"%d",&cpu_id);
    sscanf(argv[3],"%d",&ncpus);
    sscanf(argv[4],"%d",&MaxCodeLength);
  }

#ifdef USE_COMPLEX
  targetX = parseComplex(str);
#else
  sscanf(str, "%lf", &z);
  targetX = (ERR_TYPE) z;
#endif



  
#ifdef USE_COMPLEX
  if(cpu_id==1) printf("Search target:%.18lf%+.18lfI\n", creal(targetX), cimag(targetX));
#else 
  if(cpu_id==1) printf("Search target:%.18lf\n", z);
#endif

  time_t now = time (0);
  strftime (timestamp, 13, "%Y-%m-%d", localtime (&now));
  snprintf(output_filename, sizeof(output_filename), "search_log_%d_%s.txt", cpu_id,timestamp);
  search_log_file = fopen(output_filename, "w");
    if (search_log_file == NULL) {
        perror("Error opening search_log_file!");
        exit(EXIT_FAILURE);
    }

  fprintf(search_log_file,"%-20s\t%-20s\t%-20s\t%s\t%22s\t%-24s\t%-24s\t%s\t%-27s\t%-26s\t%s\n","Counter j","Code number k1","Formula number k2",
" ULP ", "Error/DBL_EPS", "Re(X)","Im(X)","cpu_id","Short code","Timestamp", "Full RPN code");

  setlinebuf(stdout); //disable 4kB stdout buffer

  //printf("DEBUG: z = %.18lf\n", targetX);
  //printf("DEBUG: cpu_id = %d\n", cpu_id);
  //printf("DEBUG: ncpus  = %d\n", ncpus);
  //printf("DEBUG: K  = %d\n", MaxCodeLength);
  srand(cpu_id);
  unsigned long long int miliard = 1000000000ULL; 
  unsigned long long int found_check_period = miliard + ( (unsigned long long int) rand() );
  //printf("DEBUG: check period = %llu\n", found_check_period);
  //printf("DEBUG: RAND_MAX = %d\n", RAND_MAX);

  best  = MAX_NUMBER;
  
  j=cpu_id;
  for(K=1;K<=MaxCodeLength;K++){
    kMAX=ipow(INSTR_NUM,K);
    for(k=cpu_id;k<kMAX;k=k+ncpus)
    {		
	j=j+ncpus;
    
      if(  ((j-cpu_id)/ncpus) % (found_check_period) == 0 ){ //Op. order %, !=/==, &&
          //printf("DEBUG: checking flagfile from thread %d at j=%llu\n",cpu_id,j);  
          flagfile = fopen("found.txt","r");
          if (flagfile == NULL){ // Handle the case where the file doesn't exist or couldn't be opened
            printf("File found.txt do not exist. Thread %d exit.\n",cpu_id); 
            exit(0);
          }
          if (fscanf(flagfile, "%d", &omp_cancel_flag) != 1) { // Handle error if fscanf fails to read an integer
            printf("Unable to read found.txt Thread %d exit.\n",cpu_id); 
            fclose(flagfile); 
            exit(0);
          }
          if(omp_cancel_flag==1){ 
            printf("EXIT JOB %d\n",cpu_id);  
            fclose(flagfile); 
            fclose(search_log_file); 
            exit(0); 
          }
          fclose(flagfile);
	  }
	

    /* Convert number 'k' into string 'amino' in base-n number of length 'K' including leading zeros */
    itoa(k, amino, n, K);
        
    test = checkSyntax (amino, K); //check if RPN code is valid 
    if (!test) continue;

	k1++;

#ifdef USE_COMPLEX         
    computedX = CONSTANT(amino, K);
	if (IS_NAN(creal(computedX)) || IS_NAN(cimag(computedX))) continue;  // Skip NaN
	k2++;
    //var = ABS( computedX/targetX - ONE );	  
    var = rankFunc(computedX, targetX);
#else
    computedX = CONSTANT(amino, K);
	if (IS_NAN(computedX)) continue;  // Skip NaN
    k2++;
    var = rankFunc(computedX, targetX);  
#endif

		
					  
    
    if(var<best) 
     {
      best = var;

      ULP = compute_ULP_distance(computedX, targetX);

#ifdef USE_COMPLEX           
     fprintf(search_log_file,"%20llu\t%20llu\t%20llu\t%20lu\t%22.17lf\t%24.18lf\t%24.18lf\t%-6d\t%-28s\t",j,k1,k2,ULP, best, creal(computedX),cimag(computedX),cpu_id,amino);
#else
     fprintf(search_log_file,"%20llu\t%20llu\t%20llu\t%20lu\t%22.17lf\t%24.18lf\t%24.18lf\t%-6d\t%-28s\t",j,k1,k2,ULP, best, computedX,                    0.0,cpu_id,amino);
#endif
      
      time_t now = time (0);
      strftime (timestamp, 26, "%Y-%m-%d %H:%M:%S.000", localtime (&now));
      fprintf(search_log_file, "%s\t", timestamp);
      print_code_mathematica(amino,K,RPN_full_Code);
      fprintf(search_log_file,"{%s}\n",RPN_full_Code);



	 }
  
	 if(best<=EPS_MAX*EPSILON) //jezeli znalazl, wychodzi z petli i zapisuje plik dla innych procesow
	 {
		  
          printf("\nConstant recognized by thread %d:\tError in $MachineEps=%le\tCode number=%llu\tSHORT CODE:\t%s\n",cpu_id, best/DBL_EPSILON,j,amino);

          ULP = compute_ULP_distance(computedX, targetX);

          printf("Total valid formulae [all codes] tested by thread %d:\t%llu [%llu]\n",cpu_id,k2,k1);
          printf("Minimal error in ULP=%lu\n", ULP );
#ifdef USE_COMPLEX
	      printf("Re=%.18lf\n",creal(computedX));
	      printf("Im=%.18lf\n",cimag(computedX));
#else
	      printf("Re=%.18lf\n",computedX);
	      printf("Im=%.18lf\n",0.0);
#endif
	      
          
          print_code_mathematica(amino,K,RPN_full_Code);
          printf("RPN CODE:\t{%s}\t",RPN_full_Code);
	      printf("\n\n");

          omp_cancel_flag = 1;
		  flagfile = fopen("found.txt","w");
          fprintf(flagfile, "%d", omp_cancel_flag);
          fclose(flagfile);
          fclose(search_log_file);

		  exit(0);//break;
     }

    
	
    }
  }
  //printf("DEBUG: from thread %d \t j   = %llu\n", cpu_id, j); 
  //printf("DEBUG: from thread %d \t k   = %llu\n", cpu_id, k); 
  //printf("DEBUG: from thread %d \t k1  = %llu\n", cpu_id, k1); 
  //printf("DEBUG: from thread %d \t k2  = %llu\n", cpu_id, k2); 
 
  printf("END of search for thread %d\n\n", cpu_id);
  
  fclose(search_log_file);

return 0;

}
