/* CALC_EXAMPLE.h - Example custom calculator
 * 
 * Author: Andrzej Odrzywolek
 * Date: December 28, 2025
 *
 * An example custom calculator demonstrating how to define
 * your own instruction set. Copy and modify for your needs.
 *
 * Buttons: 11 total
 *   - 4 constants: π, e, 1, 2
 *   - 4 unary: log, exp, sqrt, sqr  
 *   - 3 binary: -, ×, ^
 */

#ifndef CALC_EXAMPLE_H
#define CALC_EXAMPLE_H

#define _USE_MATH_DEFINES
#include <math.h>
#include "vsearch_RPN_core.h"
#include "math2.h"

/* Constants */
static const ConstOp EXAMPLE_CONSTS[] = {
    { M_PI, "PI"    },
    { M_E,  "EULER" },
    { 1.0,  "ONE"   },
    { 2.0,  "TWO"   }
};
#define EXAMPLE_N_CONST ((int)ARRAY_SIZE(EXAMPLE_CONSTS))

/* Unary functions */
static const UnaryOp EXAMPLE_FUNCS[] = {
    { log,  "LOG"  },
    { exp,  "EXP"  },
    { sqrt, "SQRT" },
    { sqr,  "SQR"  }   /* from math2.h */
};
#define EXAMPLE_N_UNARY ((int)ARRAY_SIZE(EXAMPLE_FUNCS))

/* Binary operators */
static const BinaryOp EXAMPLE_OPS[] = {
    { subtract, "SUBTRACT" },   /* from math2.h */
    { times,    "TIMES"    },   /* from math2.h */
    { pow,      "POWER"    }
};
#define EXAMPLE_N_BINARY ((int)ARRAY_SIZE(EXAMPLE_OPS))

#define EXAMPLE_N_TOTAL (EXAMPLE_N_CONST + EXAMPLE_N_UNARY + EXAMPLE_N_BINARY)

#endif /* CALC_EXAMPLE_H */
