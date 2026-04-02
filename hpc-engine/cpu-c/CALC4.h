/* CALC4.h - 36-button scientific RPN calculator definition
 * 
 * Author: Andrzej Odrzywolek
 * Date: January 2, 2025
 *
 * This file defines the standard CALC4 calculator:
 *   - 13 constants
 *   - 18 unary functions
 *   - 5 binary operators
 *   Total: 36 "buttons"
 *
 * Usage:
 *   #include "CALC4.h"
 *   
 *   vsearch_core(MODE_CONSTANT, data, 1, MinK, MaxK, cpu_id, ncpus,
 *       CALC4_CONSTS, CALC4_N_CONST,
 *       CALC4_FUNCS,  CALC4_N_UNARY,
 *       CALC4_OPS,    CALC4_N_BINARY,
 *       ERROR_REL, COMPARE_STRICT);
 *
 * This is the "master" calculator for string-based operator selection
 * in the WASM wrapper.
 */

#ifndef CALC4_H
#define CALC4_H

#define _USE_MATH_DEFINES
#include <math.h>
#include "vsearch_RPN_core.h"
#include "math2.h"

/* ============================================================================
 * CONSTANTS (13)
 * 
 * Mathematical constants and small integers frequently used in formulas.
 * Order matters for string-based lookup in WASM wrapper.
 * ============================================================================ */

static const ConstOp CALC4_CONSTS[] = {
    { M_PI,                                  "PI"          },
    { M_E,                                   "EULER"       },
    { -1.0,                                  "NEG"         },
    { 1.61803398874989484820458683436563812, "GOLDENRATIO" },
    { 1.0,                                   "ONE"         },
    { 2.0,                                   "TWO"         },
    { 3.0,                                   "THREE"       },
    { 4.0,                                   "FOUR"        },
    { 5.0,                                   "FIVE"        },
    { 6.0,                                   "SIX"         },
    { 7.0,                                   "SEVEN"       },
    { 8.0,                                   "EIGHT"       },
    { 9.0,                                   "NINE"        }
};

#define CALC4_N_CONST ((int)ARRAY_SIZE(CALC4_CONSTS))

/* ============================================================================
 * UNARY FUNCTIONS (18)
 * 
 * Standard mathematical functions operating on a single value.
 * Includes logarithms, exponentials, trigonometric, and hyperbolic functions.
 * ============================================================================ */

static const UnaryOp CALC4_FUNCS[] = {
    { log,    "LOG"      },
    { exp,    "EXP"      },
    { inv,    "INV"      },   /* from math2.h: 1/x */
    { tgamma, "GAMMA"    },
    { sqrt,   "SQRT"     },
    { sqr,    "SQR"      },   /* from math2.h: x*x */
    { sin,    "SIN"      },
    { asin,   "ARCSIN"   },
    { cos,    "COS"      },
    { acos,   "ARCCOS"   },
    { tan,    "TAN"      },
    { atan,   "ARCTAN"   },
    { sinh,   "SINH"     },
    { asinh,  "ARCSINH"  },
    { cosh,   "COSH"     },
    { acosh,  "ARCCOSH"  },
    { tanh,   "TANH"     },
    { atanh,  "ARCTANH"  }
};

#define CALC4_N_UNARY ((int)ARRAY_SIZE(CALC4_FUNCS))

/* ============================================================================
 * BINARY OPERATORS (5)
 * 
 * Standard binary operations. Note: in RPN evaluation, operands are
 * popped in reverse order, so the function signature is func(b, a)
 * where 'a' was pushed first and 'b' second.
 * ============================================================================ */

static const BinaryOp CALC4_OPS[] = {
    { plus,     "PLUS"     },   /* from math2.h */
    { times,    "TIMES"    },   /* from math2.h */
    { subtract, "SUBTRACT" },   /* from math2.h */
    { divide,   "DIVIDE"   },   /* from math2.h */
    { pow,      "POWER"    }
};

#define CALC4_N_BINARY ((int)ARRAY_SIZE(CALC4_OPS))

/* ============================================================================
 * TOTAL BUTTON COUNT
 * ============================================================================ */

#define CALC4_N_TOTAL (CALC4_N_CONST + CALC4_N_UNARY + CALC4_N_BINARY)

/* 
 * CALC4 should have exactly 36 buttons (13 + 18 + 5).
 * Verify at runtime with: assert(CALC4_N_TOTAL == 36);
 */

#endif /* CALC4_H */
