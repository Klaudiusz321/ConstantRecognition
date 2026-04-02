/* Andrzej Odrzywolek, 04.01.2025, andrzej.odrzywolek@uj.edu.pl */
#include "../WASM/ConstantRecognition_function2_for_WASM.c"
#include "Benchmark_CASIO_HL-815L_K7_L2.c"

  // Declare a volatile pointer to store the result
  volatile char *sink;

int main(int argc, char** argv)
{

  int num = (sizeof(benchmark)/8);
  int depth,cpu_id,ncpus;


  //printf("num=%d\n",num);

  if(!(argv[1]==NULL))
  {
    sscanf(argv[1],"%d" , &depth);
    sscanf(argv[2],"%d",&cpu_id);
    sscanf(argv[3],"%d",&ncpus);
  }

  //printf("K<=%d\tcpu_id=%d\tncpus=%d\n",depth,cpu_id,ncpus);


  for(int ii=0;ii<num;ii++)
   {
    printf("%s\n",search_RPN(benchmark[ii], 0.0, 1, depth,cpu_id,ncpus));
    //sink = search_RPN(benchmark[ii], 0.0, 1, depth,cpu_id,ncpus);
    //printf("K<=%d\tcpu_id=%d\tncpus=%d\n",depth,cpu_id,ncpus);
    //printf("%d\tz=%lf\n", ii, benchmark[ii]);
   } 


    //printf("%s\n",search_RPN(9997.0, 0.0, 1, depth, cpu_id, ncpus));
  
}
