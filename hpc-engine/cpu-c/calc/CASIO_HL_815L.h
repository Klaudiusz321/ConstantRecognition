/* Custom calculator based on CASIO HL-815L
 * 
 * Author: Andrzej Odrzywolek
 * Date: Jan 1, 2026
 *
 * Custom calculator demonstrating how to define
 * standard office calculator
 *
 * Buttons: 17 total
 *   - 10 constants: 0,1,2,3,4,5,6,7,8,9 (just digits)
 *   - 2 unary: sqrt, %
 *   - 5 binary: +,-,*,/ and digit concatenation, e.g. 9||9=99
 *   TODO: add concatenation with decimal dot, e.g. dot(3,14) = 3 . 14 -> 3.14
 */

#ifndef CASIO_HL_815L_H
#define CASIO_HL_815L_H

#define _USE_MATH_DEFINES
#include <math.h>
#include "../vsearch_RPN_core.h"
#include "../math2.h"

/* Constants */
static const ConstOp CONSTS[] = {
    { 0.0,  "ZERO"  },
    { 1.0,  "ONE"   },
    { 2.0,  "TWO"   },
    { 3.0,  "THREE" },
    { 4.0,  "FOUR"  },
    { 5.0,  "FIVE"  },
    { 6.0,  "SIX"   },
    { 7.0,  "SEVEN" },
    { 8.0,  "EIGHT" },
    { 9.0,  "NINE"  }
};
#define N_CONST ((int)ARRAY_SIZE(CONSTS))

/* Unary functions */
static const UnaryOp FUNCS[] = {
    { sqrt, "SQRT" },
    { percent,  "PERCENT"  }   // Can we use just % as name?
};
#define N_UNARY ((int)ARRAY_SIZE(FUNCS))

/* Binary operators */
static const BinaryOp OPS[] = {
    { plus,     "PLUS"     },  
    { subtract, "SUBTRACT" },
    { times,    "TIMES"    }, 
    { divide,   "DIVIDE"   },
    { concat,   "II"}
};
#define N_BINARY ((int)ARRAY_SIZE(OPS))

#define N_TOTAL (N_CONST + N_UNARY + N_BINARY)

#endif /* CASIO_HL_815L_H */
