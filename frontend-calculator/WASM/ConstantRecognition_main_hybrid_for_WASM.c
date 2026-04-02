/* Main function for Hybrid FP32+FP64 WASM
   Based on ConstantRecognition_main_function_for_WASM.c
   
   Klaudiusz, December 2025
*/

#include "ConstantRecognition_hybrid_for_WASM.c"

int main(int argc, char** argv)
{
  double z;
  int K, cpu_id, ncpus;

  if (!(argv[1] == NULL))
  {
    sscanf(argv[1], "%lf", &z);
    sscanf(argv[2], "%d", &K);
    sscanf(argv[3], "%d", &cpu_id);
    sscanf(argv[4], "%d", &ncpus);
  }

  // Wywołanie hybrydowej funkcji search_RPN_hybrid zamiast search_RPN
  printf("%s\n", search_RPN_hybrid(z, 0.005, 1, K, cpu_id, ncpus));
  
  return 0;
}
