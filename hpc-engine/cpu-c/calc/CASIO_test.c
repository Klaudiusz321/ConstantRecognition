/* Simple test of CASIO HL-815H calculator.

Author: Andrzej Odrzywolek
Date: January 2, 2025

Notice: search fails to recognize sqrt(137), which CASIO computes in 8-digit precision,
despite high "COMPRESSION_RATIO":0.962924, visible error drop (accuracy jump)
from "REL_ERR":2.99446615360965396e-04
to   "REL_ERR":7.78080346819365332e-08,
i.e. 3850x.  

Probability could be estimated from counters:
"total_ternary":1092, "valid_ternary":38, "evaluations":606130
i.e. what is probability of randomly hitting target of  11.704699 +- 0.000001 after 606130 tries. 

Runtime, icx -02, AMD Ryzen 5900X, Windows = 1.4 seconds.

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../vsearch_RPN_core.h"
#include "CASIO_HL_815L.h"


int main(int argc, char** argv) {

    
    


    char* result_json = search_constant(
    11.704699,  //sqrt(137) 
     0.000001,
    1, 6,
    0, 1,
    CONSTS, N_CONST,
    FUNCS,  N_UNARY,
    OPS,    N_BINARY,
    ERROR_REL,
    COMPARE_STRICT);
    
    printf("%s", result_json);

    free(result_json);



    
    return 0;
}
